//===--- ada_runtime.c - Minimal Ada 2012 runtime (P3 skeleton) ----*- C -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Minimal in-tree Ada runtime (D4: no libgnat). This P3 skeleton provides:
//
//   * main registration/dispatch: each compiled Ada procedure library unit
//     registers itself via llvm.global_ctors; main() runs the first
//     registered procedure (link-order convention: first input is main).
//   * exceptions: a handler stack with a hand-rolled context switch
//     (__ada_push_handler / __ada_setctx / __ada_raise /
//     __ada_pop_handler). LLVM EH + Ada.Exceptions replace this in P5.
//   * allocation: plain malloc/free wrappers; pool-based allocation in P5.
//
// Everything lives in one C file on purpose: the linker pulls the single
// object in (via main or __ada_main_register) and gets the whole runtime.
//
//===----------------------------------------------------------------------===//

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

//===----------------------------------------------------------------------===//
// Main registration and dispatch
//===----------------------------------------------------------------------===//

#define ADA_MAX_UNITS 256

typedef void (*AdaMainProc)(void);

static AdaMainProc __ada_main_table[ADA_MAX_UNITS];
static int __ada_main_count = 0;

// Called from the llvm.global_ctors constructor of each compiled Ada
// procedure library unit.
void __ada_main_register(void *Proc) {
  AdaMainProc P;
  // memcpy keeps this C-conformant (a void * to function-pointer cast is
  // not); it works on every ABI in practice.
  memcpy(&P, &Proc, sizeof(P));
  if (__ada_main_count < ADA_MAX_UNITS)
    __ada_main_table[__ada_main_count++] = P;
}

// The process entry point. Convention: the first registered procedure is
// the main unit (object link order == source order on the clang-ada line).
int main(void) {
  if (__ada_main_count > 0)
    __ada_main_table[0]();
  return 0;
}

//===----------------------------------------------------------------------===//
// Buffered standard output
//
// Text_IO writes go through a small buffer to cut down on write(2) syscalls
// (P11 runtime performance). The buffer is flushed at normal exit (atexit)
// and before an unhandled exception aborts, so output is never lost.
//===----------------------------------------------------------------------===//

#define ADA_OBUF_SIZE 8192
static char __ada_obuf[ADA_OBUF_SIZE];
static size_t __ada_olen = 0;

void __ada_flush(void) {
  if (__ada_olen) {
    ssize_t Ignored = write(STDOUT_FILENO, __ada_obuf, __ada_olen);
    (void)Ignored;
    __ada_olen = 0;
  }
}

static void __ada_owrite(const char *S, size_t N) {
  while (N) {
    size_t Space = ADA_OBUF_SIZE - __ada_olen;
    size_t Chunk = N < Space ? N : Space;
    memcpy(__ada_obuf + __ada_olen, S, Chunk);
    __ada_olen += Chunk;
    S += Chunk;
    N -= Chunk;
    if (__ada_olen == ADA_OBUF_SIZE)
      __ada_flush();
  }
}

__attribute__((constructor)) static void __ada_io_init(void) {
  atexit(__ada_flush);
}

//===----------------------------------------------------------------------===//
// Exceptions (hand-rolled context switch; replaced by LLVM EH in P5)
//
// We do NOT use glibc's setjmp/longjmp (nor the GCC builtins, which lower
// to the same canaried jmp_buf machinery): on the glibc 2.44 build in this
// devshell, a cross-function longjmp resumes with rbp restored 8 bytes low.
// Core-dump forensics: with the correct rbp, the catching frame's
// `leave; ret` epilogue pops the push site's return address and the handler
// runs; with the 8-low rbp, `leave` sets rsp 8 low and `ret` pops a stale
// return address, so execution silently resumes in the middle of a dead
// frame. Same-function setjmp/longjmp works, which is what made this
// subtle. See AGENTS.md section 10 for the full record.
//
// Instead the runtime switches context with a minimal asm trampoline that
// saves exactly the state the SysV calling convention promises to a
// setjmp/longjmp pair: rbx, r12-r15, rbp, rsp, rip. The trampolines are
// naked functions (no compiler prologue or epilogue), so at entry (%rsp)
// is the return address, and the saved (Rsp, Rip) pair is the machine
// state right after the call returned. The jump side restores the
// registers, loads Value into eax (the setjmp convention: nonzero on
// resume) and jumps to Rip.
//
// Frame accounting (why this is safe): the capture must happen as a
// direct call from the function that contains the try -- the generated
// code calls __ada_setctx itself, right after the bookkeeping-only
// __ada_push_handler -- so the saved Rip points into a live frame.
// (Capturing inside a helper that returns before the raise is wrong: by
// the time the raise happens the helper has returned, its frame is dead,
// and the caller's return slot at the same depth may already be reused
// by the next call, so the resume lands in the middle of dead code.)
// Calls made during the try body only push below the capture-point Rsp
// and return, so they cannot overwrite the captured state. On the jump
// path the restored Rsp/Rip are the machine state right after the
// __ada_setctx call in the enclosing subprogram, and the resumed code
// sees eax == 1, i.e. the call "returned" 1. Locals of the enclosing
// subprogram are indeterminate after the jump, per the C 7.14.2.1
// setjmp contract; the generated code does not rely on them (it re-reads
// __ada_raised_name and the handler state). Only rbx, r12-r15, rbp, rsp,
// rip and eax are valid on resume.
//===----------------------------------------------------------------------===//

#define ADA_MAX_HANDLERS 256

typedef struct {
  uint64_t Rbx, R12, R13, R14, R15;
  uint64_t Rbp, Rsp, Rip;
} AdaCtx;

typedef struct {
  AdaCtx Ctx;
  int Active;
} AdaHandler;

static AdaHandler __ada_handlers[ADA_MAX_HANDLERS];
static int __ada_handler_top = 0;

// Captures the caller's context into Ctx. Always returns 0 on the first
// pass; on the resume path this function never returns at all --
// __ada_longctx jumps to the saved Rip with eax = 1, so to the generated
// code the call appears to return 1. Exported (not static): the generated
// code calls it directly, right after __ada_push_handler, so the saved
// Rip points into the live frame of the subprogram containing the try.
// Naked: the asm is the entire function, so there is no prologue between
// the call and the capture and (%rsp) at entry is the return address.
__attribute__((naked)) int __ada_setctx(AdaCtx *) {
  __asm__ __volatile__(
      "movq %%rbx, 0(%%rdi)\n\t"
      "movq %%r12, 8(%%rdi)\n\t"
      "movq %%r13, 16(%%rdi)\n\t"
      "movq %%r14, 24(%%rdi)\n\t"
      "movq %%r15, 32(%%rdi)\n\t"
      "movq %%rbp, 40(%%rdi)\n\t"
      "movq (%%rsp), %%rax\n\t"  // Rip = return address
      "movq %%rax, 56(%%rdi)\n\t"
      "leaq 8(%%rsp), %%rax\n\t" // Rsp = stack state once the ret below
      "movq %%rax, 48(%%rdi)\n\t" // has popped the return address
      "xorl %%eax, %%eax\n\t"    // return 0 on the first pass
      "ret\n\t"
      :
      : /* rdi = Ctx, per the SysV ABI */
      : "rax", "memory");
}

// Restores the captured context and jumps to it; does not return. At the
// resume point, eax holds Value (the setjmp convention: nonzero).
__attribute__((naked)) static void __ada_longctx(const AdaCtx *, int) {
  __asm__ __volatile__(
      "movq 0(%%rdi), %%rbx\n\t"
      "movq 8(%%rdi), %%r12\n\t"
      "movq 16(%%rdi), %%r13\n\t"
      "movq 24(%%rdi), %%r14\n\t"
      "movq 32(%%rdi), %%r15\n\t"
      "movq 40(%%rdi), %%rbp\n\t"
      "movq 48(%%rdi), %%rsp\n\t"
      "movq 56(%%rdi), %%r11\n\t" // Rip into r11 first: the eax load below
                                  // must not be clobbered (earlier drafts
                                  // loaded Rip into rax and the resume
                                  // address overwrote the return value)
      "movl %%esi, %%eax\n\t"     // Value = 2nd argument, SysV ABI
      "jmp *%%r11\n\t"
      :
      : /* rdi = Ctx, esi = Value, per the SysV ABI */
      : "rax", "r11", "memory");
}

// Registers a try-block handler (bookkeeping only) and returns the slot.
// The generated code passes the slot to __ada_setctx immediately after, so
// the context capture happens in the frame of the subprogram containing
// the try (see the frame-accounting note above). The slot stays active
// until one of:
//   * the try completes normally: the generated code calls
//     __ada_pop_handler (pops the slot);
//   * a raise jumps through it: __ada_raise deactivates the slot before
//     jumping, so raises inside the except block propagate to the
//     enclosing handler. The deactivated slot is then stale at its index;
//     the dispatch scan skips inactive slots and the slot is overwritten
//     by the next push once top moves on. (Consequence: a subprogram that
//     raises-and-catches more than ADA_MAX_HANDLERS times without ever
//     returning aborts; acceptable for the P3 skeleton.)
void *__ada_push_handler(void) {
  if (__ada_handler_top >= ADA_MAX_HANDLERS)
    abort();
  AdaHandler *H = &__ada_handlers[__ada_handler_top];
  H->Active = 1;
  __ada_handler_top++;
  return H;
}

// Removes the innermost handler (normal completion of a try block).
void __ada_pop_handler(void) {
  if (__ada_handler_top > 0)
    __ada_handlers[--__ada_handler_top].Active = 0;
}

// Name of the exception currently in flight (P3 skeleton: exceptions are
// identified by name only; Ada.Exceptions objects arrive in P5).
static const char *__ada_raised_name = "";

// Raises the named exception. Jumps to the innermost active handler, or
// terminates the program when none exists.
void __ada_raise(const char *Name) {
  int I;
  __ada_raised_name = Name ? Name : "";
  for (I = __ada_handler_top - 1; I >= 0; --I)
    if (__ada_handlers[I].Active) {
      // Deactivate before jumping: the except block runs with this
      // handler logically removed, so a raise inside it (or the implicit
      // re-raise at the end of an unmatched except part) propagates to
      // the enclosing handler instead of looping back here.
      __ada_handlers[I].Active = 0;
      __ada_longctx(&__ada_handlers[I].Ctx, 1);
    }
  __ada_flush();
  fprintf(stderr, "ada: unhandled exception: %s\n", Name);
  abort();
}

// Name of the exception currently in flight; valid while unwinding and
// inside an except part.
const char *__ada_current_exception(void) { return __ada_raised_name; }

// Whether the exception currently in flight has the given name.
int __ada_exception_match(const char *Name) {
  return __ada_raised_name != NULL && Name != NULL &&
         strcmp(__ada_raised_name, Name) == 0;
}

//===----------------------------------------------------------------------===//
// Pool-based allocator (RM A.14)
//===----------------------------------------------------------------------===//
//
// Simple fixed-size block allocator. The pool is a single large allocation
// that is divided into blocks. This is sufficient for the Ada subset we
// support (no fragmentation, no free-list management).
//
// For production use, this would be replaced by a more sophisticated
// allocator (e.g., dlmalloc, jemalloc, or the system allocator with
// pool headers).

#define ADA_POOL_BLOCK_SIZE 64
#define ADA_POOL_SIZE (1024 * 1024)

static char *__ada_pool = NULL;
static size_t __ada_pool_offset = 0;

void __ada_pool_init(void) {
  if (!__ada_pool) {
    __ada_pool = (char *)malloc(ADA_POOL_SIZE);
    if (!__ada_pool)
      abort();
    __ada_pool_offset = 0;
  }
}

void *__ada_pool_allocate(size_t Size) {
  if (!__ada_pool)
    __ada_pool_init();

  // Align to block size
  size_t Aligned = (Size + ADA_POOL_BLOCK_SIZE - 1) / ADA_POOL_BLOCK_SIZE *
                  ADA_POOL_BLOCK_SIZE;
  if (__ada_pool_offset + Aligned > ADA_POOL_SIZE) {
    fprintf(stderr, "ada: pool allocation overflow\n");
    abort();
  }

  void *P = __ada_pool + __ada_pool_offset;
  __ada_pool_offset += Aligned;
  return P;
}

void __ada_pool_deallocate(void *P) {
  // Pool deallocation is a no-op (blocks are reclaimed when the pool is freed).
  // In a real implementation, this would return the block to a free list.
}

void __ada_pool_free(void) {
  if (__ada_pool) {
    free(__ada_pool);
    __ada_pool = NULL;
    __ada_pool_offset = 0;
  }
}

// Ada access type allocation uses the pool.
void *__ada_allocate(size_t Size) { return __ada_pool_allocate(Size); }

void __ada_deallocate(void *P) { __ada_pool_deallocate(P); }

// Ada.Text_IO minimum (RM 11.3)
void __ada_put(const char *S) {
  __ada_owrite(S, strlen(S));
}

void __ada_print(const char *S) {
  __ada_owrite(S, strlen(S));
  __ada_owrite("\n", 1);
}

void __ada_new_line(void) {
  __ada_owrite("\n", 1);
}

void __ada_print_int(int N) {
  char Buf[32];
  int I = 0;
  if (N == 0) {
    Buf[0] = '0';
    Buf[1] = '\0';
  } else {
    int Neg = N < 0;
    if (Neg) N = -N;
    while (N > 0) {
      Buf[I++] = '0' + (N % 10);
      N = N / 10;
    }
    if (Neg) Buf[I++] = '-';
    Buf[I] = '\0';
    // Reverse
    int J = 0;
    int K = I - 1;
    while (J < K) {
      char Tmp = Buf[J];
      Buf[J] = Buf[K];
      Buf[K] = Tmp;
      J++;
      K--;
    }
  }
  __ada_owrite(Buf, I);
}

void __ada_print_float(double F) {
  char Buf[64];
  int N = snprintf(Buf, sizeof(Buf), "%f", F);
  if (N > 0)
    __ada_owrite(Buf, (size_t)N);
}

void __ada_print_int_new_line(int N) {
  __ada_print_int(N);
  __ada_new_line();
}

void __ada_print_float_new_line(double F) {
  __ada_print_float(F);
  __ada_new_line();
}

// Ada.Integer_Text_IO minimum (RM 11.4)
void __ada_integer_put(int N) {
  __ada_print_int(N);
}

void __ada_integer_get(int *N) {
  // Simplified: read one integer
  char Buf[32];
  int I = 0;
  int Ch = read(STDIN_FILENO, &Buf[0], 1);
  while (Ch > 0) {
    if (Buf[0] == '\n' || Buf[0] == ' ') break;
    Buf[I++] = Buf[0];
    Ch = read(STDIN_FILENO, &Buf[0], 1);
  }
  Buf[I] = '\0';
  *N = atoi(Buf);
}

// Ada.Float_Text_IO minimum (RM 11.4)
void __ada_float_put(double F) {
  __ada_print_float(F);
}

void __ada_float_get(double *F) {
  // Simplified: read one float
  char Buf[64];
  int I = 0;
  int Ch = read(STDIN_FILENO, &Buf[0], 1);
  while (Ch > 0) {
    if (Buf[0] == '\n' || Buf[0] == ' ') break;
    Buf[I++] = Buf[0];
    Ch = read(STDIN_FILENO, &Buf[0], 1);
  }
  Buf[I] = '\0';
  *F = atof(Buf);
}

//===----------------------------------------------------------------------===//
// Ada.Calendar minimum (RM 9.6)
//===----------------------------------------------------------------------===//

// Seconds since the Unix epoch (Ada.Calendar.Clock).
double __ada_clock(void) {
  struct timespec Ts;
  clock_gettime(CLOCK_REALTIME, &Ts);
  return (double)Ts.tv_sec + (double)Ts.tv_nsec / 1.0e9;
}

//===----------------------------------------------------------------------===//
// Ada.Strings minimum (RM A.4.3)
//===----------------------------------------------------------------------===//

// Length of a bounded String value (NUL-terminated in this runtime).
int __ada_string_length(const char *S) {
  return S ? (int)strlen(S) : 0;
}

//===----------------------------------------------------------------------===//
// Ada.Environment_Variables minimum (RM A.15)
//===----------------------------------------------------------------------===//

// Value(Name); returns "" when unset.
const char *__ada_env_value(const char *Name) {
  const char *V = getenv(Name);
  return V ? V : "";
}

// Exists(Name).
int __ada_env_exists(const char *Name) {
  return getenv(Name) != NULL;
}

// Ada.Tags minimum (RM 13.9) - RTTI support
// Tag values are stored in the first 8 bytes of tagged objects as a pointer
// to a tag global variable. The tag global variable's address serves as the
// tag identity.

// __ada_tag_of returns the tag (tag global address) of an object
void *__ada_tag_of(void *Object) {
  void **TagSlot = (void **)Object;
  return *TagSlot;
}

// Tag names are registered at runtime; this is a simplified implementation
// that just returns a null pointer. Full implementation requires a registry.
static char *TagNames[256];
static int TagCount = 0;

void __ada_register_tag(void *Tag, const char *Name) {
  if (TagCount < 256) {
    TagNames[TagCount] = (char *)Name;
    TagCount++;
  }
}

// __ada_tag_name returns the name of a tag
const char *__ada_tag_name(void *Tag) {
  // Linear search through registered tags
  for (int I = 0; I < TagCount; I++) {
    // Compare tag addresses
    if (Tag == (void *)TagNames[I]) {
      return TagNames[I];
    }
  }
  return "";
}

// Ada tasking (RM 9) - pthread-based implementation
// Task control block
typedef struct AdaTask {
  pthread_t Thread;
  void (*Entry)(void);
  void *Arg;
  int Active;
  pthread_mutex_t Mutex;
  pthread_cond_t Cond;
  int EntryCalled;
  pthread_mutex_t CallMutex;
  pthread_cond_t CallCond;
  // Exception propagation (P10)
  void *RaisedException;
  int Joining;
} AdaTask;

// Create a new task (spawn a pthread)
AdaTask *__ada_create_task(void (*Entry)(void), void *Arg) {
  AdaTask *Task = (AdaTask *)malloc(sizeof(AdaTask));
  Task->Entry = Entry;
  Task->Arg = Arg;
  Task->Active = 1;
  Task->EntryCalled = 0;
  pthread_mutex_init(&Task->Mutex, NULL);
  pthread_cond_init(&Task->Cond, NULL);
  pthread_mutex_init(&Task->CallMutex, NULL);
  pthread_cond_init(&Task->CallCond, NULL);
  pthread_create(&Task->Thread, NULL, (void *(*)(void *))Entry, Arg);
  return Task;
}

// Wait for a task to complete
void __ada_join_task(AdaTask *Task) {
  Task->Joining = 1;
  pthread_join(Task->Thread, NULL);
  Task->Active = 0;
  // If the task raised an exception, re-raise it (P10)
  if (Task->RaisedException) {
    __ada_raise((char *)Task->RaisedException);
  }
  pthread_mutex_destroy(&Task->Mutex);
  pthread_cond_destroy(&Task->Cond);
  pthread_mutex_destroy(&Task->CallMutex);
  pthread_cond_destroy(&Task->CallCond);
  free(Task);
}

// Signal that an entry is ready to be accepted
void __ada_entry_signal(AdaTask *Task) {
  pthread_cond_signal(&Task->Cond);
}

// Wait for an entry call
void __ada_entry_wait(AdaTask *Task) {
  pthread_mutex_lock(&Task->Mutex);
  pthread_cond_wait(&Task->Cond, &Task->Mutex);
  pthread_mutex_unlock(&Task->Mutex);
}

// Signal that a task has raised an exception (for propagation to caller)
void __ada_task_raise(AdaTask *Task, const char *Name) {
  Task->RaisedException = (void *)Name;
  if (Task->Joining) {
    // Wake up the joining thread so it can re-raise
    pthread_cond_signal(&Task->Cond);
  }
}

// Abort a task (terminate it immediately)
void __ada_abort_task(AdaTask *Task) {
  Task->Active = 0;
  pthread_cancel(Task->Thread);
}

// Delay for a given number of seconds (RM 9.6)
void __ada_delay(double Seconds) {
  struct timespec ts;
  ts.tv_sec = (time_t)Seconds;
  ts.tv_nsec = (long)((Seconds - (double)ts.tv_sec) * 1000000000.0);
  nanosleep(&ts, NULL);
}

// Requeue a task on an entry (RM 9.6.2)
// Simplified stub - full implementation would involve entry queue management
void __ada_requeue(const char *EntryName) {
  // In a full implementation, this would:
  // 1. Remove the caller from the current entry queue
  // 2. Place the caller on the specified entry queue
  // 3. Handle the 'with abort' semantics
  // For now, this is a no-op that logs the requeue operation.
  (void)EntryName;
}

// Entry call parameter storage
#define ADA_MAX_ENTRY_PARAMS 10
int __ada_entry_params[ADA_MAX_ENTRY_PARAMS];

// Entry call: signal the task and wait for accept to complete
void __ada_entry_call(AdaTask *Task, int NumParams, int *Params) {
  // Store parameters for the accept statement
  if (NumParams <= ADA_MAX_ENTRY_PARAMS) {
    for (int i = 0; i < NumParams; i++) {
      __ada_entry_params[i] = Params[i];
    }
  }
  
  // Signal the task that an entry call has arrived
  pthread_mutex_lock(&Task->Mutex);
  Task->EntryCalled = 1;
  pthread_cond_signal(&Task->Cond);
  pthread_mutex_unlock(&Task->Mutex);
  
  // Wait for the accept to complete
  pthread_mutex_lock(&Task->CallMutex);
  pthread_cond_wait(&Task->CallCond, &Task->CallMutex);
  pthread_mutex_unlock(&Task->CallMutex);
}

// Accept: wait for entry call, execute body (caller responsibility), signal caller
void __ada_entry_accept_start(AdaTask *Task) {
  pthread_mutex_lock(&Task->Mutex);
  while (!Task->EntryCalled) {
    pthread_cond_wait(&Task->Cond, &Task->Mutex);
  }
  Task->EntryCalled = 0;
  pthread_mutex_unlock(&Task->Mutex);
}

// Accept complete: signal the caller
void __ada_entry_accept_end(AdaTask *Task) {
  pthread_cond_signal(&Task->CallCond);
}
