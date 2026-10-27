// RUN: %clang_ada -ac1 -emit-llvm %s -o - | FileCheck %s
-- Test the 'Tag attribute (Ada.Tags minimum)
procedure TestTag is
    type Shape is tagged null record;
    type Circle is new Shape;
    C : Circle;
begin
    null;
end;
// CHECK: @__ada_tag_Shape = internal constant i8 1
// CHECK: @__ada_tag_Circle = internal constant i8 2
