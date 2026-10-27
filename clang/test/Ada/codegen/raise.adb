// RUN: %clang_ada -a2012 -c %s -o %t.o | FileCheck %s

procedure RaiseOnly is
begin
   raise Constraint_Error;
end RaiseOnly;

CHECK: @[[NAME:[0-9]+]] = private unnamed_addr constant [17 x i8] c"Constraint_Error\00", align 1
CHECK: @llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @__ada_ctor_RaiseOnly, ptr null }]
CHECK: define void @RaiseOnly()
CHECK: call void @__ada_raise(ptr @[[NAME]])
CHECK-NEXT: ret void
CHECK: define internal void @__ada_ctor_RaiseOnly()
CHECK: call void @__ada_main_register(ptr @RaiseOnly)
