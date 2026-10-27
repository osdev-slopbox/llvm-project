// RUN: %clang_ada -a2012 -c %s -o %t.o | FileCheck %s

procedure LoopTest is
    I : Integer := 0;
    S : Integer := 0;
begin
    while I < 10 loop
        I := I + 1;
        S := S + I;
    end loop;
end LoopTest;

CHECK: @llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @__ada_ctor_LoopTest, ptr null }]
CHECK: define void @LoopTest()
CHECK: %I = alloca i32, align 4
CHECK: store i32 0, ptr %I, align 4
CHECK: %S = alloca i32, align 4
CHECK: store i32 0, ptr %S, align 4
CHECK: br label %while.cond
CHECK: while.cond:
CHECK: %{{.*}} = icmp slt i32 %{{.*}}, 10
CHECK: br i1 %{{.*}}, label %while.body, label %while.end
CHECK: while.body:
CHECK: %{{.*}} = add i32 %{{.*}}, 1
CHECK: store i32 %{{.*}}, ptr %I, align 4
CHECK: store i32 %{{.*}}, ptr %S, align 4
CHECK: br label %while.cond
CHECK: while.end:
CHECK: ret void
CHECK: define internal void @__ada_ctor_LoopTest()
CHECK: call void @__ada_main_register(ptr @LoopTest)
