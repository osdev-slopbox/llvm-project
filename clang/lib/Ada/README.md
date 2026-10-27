# Ada 2012 Frontend for Clang

This is the Ada 2012 compiler frontend for Clang, implementing the RM Ada 2012
language specification with GNAT as the behavioral reference.

## Quick Start

Compile an Ada program:
```sh
clang-ada -a2012 -c hello.adb -o hello.o
clang-ada -o hello hello.o
```

Compile with optimization:
```sh
clang-ada -a2012 -O2 -c hello.adb -o hello.o
```

Syntax-only check:
```sh
clang-ada -a2012 -fsyntax-only hello.adb
```

Dependency generation (for Makefiles):
```sh
clang-ada -a2012 -c hello.adb -MD -MF hello.d
```

## Supported Features

### Language Features (RM Ada 2012)

- **Types**: Integer, Float, Boolean, Character, String, access types, record types, tagged types
- **Control Flow**: if, case, while, for, loop, exit, return
- **Subprograms**: Functions, procedures, generic subprograms, generic packages
- **Tasking**: Tasks, protected objects, entry calls, accept statements, delay, requeue, abort
- **Exception Handling**: try, raise, exception handlers
- **Contracts**: Preconditions (Pre), postconditions (Post), invariants (Inv)
- **Aspects**: Contract, elaboration, linkage, storage, and tasking aspects
- **Pragmas**: Inline, Export, Import, Pack, Suppress, Check, Pure, Preelaborated, Elaborate, Elaborate_All, and more
- **Generics**: Formal parameters, generic instantiation, generic packages

### Optimization Levels

- `-O0`: No optimization (default)
- `-O1`: Basic optimization
- `-O2`: Standard optimization (equivalent to `-O`)
- `-O3`: Aggressive optimization

### Diagnostics

- `-Wall`: Enable the common warning groups
- `-Wextra`: Enable extra warnings (unused parameters, unreachable code, ...)
- `-Wpedantic`: Enable pedantic warnings
- `-Werror`: Promote warnings to errors
- `-Wno-ada-<name>` / `-Wada-<name>`: Disable/enable an individual warning
  category (e.g. `-Wno-ada-unused-variable`, `-Wno-ada-unknown-pragmas`)
- `-w`: Disable all warnings
- `-fsyntax-only`: Syntax check only

Diagnostics are contextual: parser errors name the expected token and what was
found, and an undeclared identifier is followed by a `note:` spelling
suggestion when a similar name is in scope.

## Runtime Library

The Ada frontend includes a minimal in-tree C runtime library (`libclangAdaRuntime.a`) that provides:

- Exception propagation and handling
- Memory allocation (pool-based)
- Buffered I/O operations (Ada.Text_IO, Ada.Integer_Text_IO, Ada.Float_Text_IO)
- `Ada.Calendar.Clock` (seconds since the epoch)
- `Ada.Strings.Fixed.Length`
- `Ada.Environment_Variables.Value` / `Exists`
- Tasking support (pthread-based)
- Controlled types (Initialize/Finalize)

`Put` / `Put_Line` are overloaded on the argument type, so `Put(42)`,
`Put(2.5)`, and `Put("text")` all work.

## Examples

### Hello World

```ada
with Ada.Text_IO;
use Ada.Text_IO;

procedure Hello is
begin
    Put_Line("Hello, World!");
end Hello;
```

### Generic Package

```ada
generic
    type Element is private;
    with function "<" (Left, Right : Element) return Boolean is <>;
package Generic_Stack is
    procedure Push(Item : Element);
    procedure Pop(Item : out Element);
end Generic_Stack;
```

### Tasking

```ada
procedure Producer_Consumer is
    task Producer is
        entry Ready;
    end Producer;

    task Consumer is
        entry Done;
    end Consumer;

    task body Producer is
    begin
        accept Ready;
        -- Produce items
    end Producer;

    task body Consumer is
    begin
        -- Consume items
        accept Done;
    end Consumer;
begin
    Producer.Ready;
    -- ...
    Consumer.Done;
end Producer_Consumer;
```

## Limitations

- No Ada 2022 support (deferred post-P12)
- Limited pragma support (subset of GNAT pragmas)
- No CLI compatibility with GNAT (`-gnat*` flags not supported)
- Automatic runtime linking blocked by nix environment (use `gcc` as linker)

## Comparison with GNAT

| Feature | GNAT | clang-ada |
|---------|------|-----------|
| Ada 2012 compliance | Yes | Yes |
| Tasking | Yes (POSIX threads) | Yes (POSIX threads) |
| Generics | Yes | Yes |
| Contracts | Yes | Yes |
| Optimization levels | Yes | Yes (-O0 to -O3) |
| Debugging | Yes (DWARF) | Yes (DWARF) |
| Runtime library | libgnat | libclangAdaRuntime (minimal) |
| CLI compatibility | `-gnat*` flags | No `-gnat*` flags |

## Building

```sh
cd /usr/local/src/slopbox/llvm
nix develop
cmake -S llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_TARGETS_TO_BUILD=X86 \
  -DLLVM_ENABLE_ASSERTIONS=ON
ninja -C build -j16
```

## Testing

```sh
nix develop --command bash -c "ninja -C build check-clang-ada"
```