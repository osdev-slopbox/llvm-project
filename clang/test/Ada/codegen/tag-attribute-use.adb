// RUN: %clang_ada -ac1 -emit-llvm %s -o - | FileCheck %s
-- Test using the 'Tag attribute on a tagged object
procedure TestTagUse is
    type Shape is tagged null record;
    S : Shape;
begin
    null;
end;
// CHECK: @__ada_tag_Shape = internal constant i8 1
