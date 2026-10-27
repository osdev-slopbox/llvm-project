// RUN: %clang_ada -ac1 -emit-llvm %s -o - | FileCheck %s
-- Test dynamic memory allocation with new
procedure TestNew is
    X : Integer;
begin
    X := Integer(new Integer(42));
end;
// CHECK: call ptr @__ada_allocate(i64 4)