// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 -c %s -o %t.o -emit-llvm | FileCheck %s
// RUN: %clang_ada -a2012 -c %s -o %t.o -emit-llvm | opt -passes=verify -o /dev/null

procedure Main is
   type Shape is tagged null record;
   type Circle is new Shape;
   type Square is new Shape;

   procedure Describe(S : Shape) is
   begin
      raise Constraint_Error;
   end Describe;

   procedure Describe(S : Circle) is
   begin
      raise Circle_Called;
   end Describe;

   C : Circle;
begin
   Describe(C);
end Main;

// Check that dispatch generates tag comparison blocks
// CHECK: @__ada_tag_Shape = internal constant i8 1
// CHECK: @__ada_tag_Circle = internal constant i8 2
// CHECK: dispatch.check_shape
// CHECK: dispatch.check_circle
// CHECK: dispatch.call_circle
// CHECK: call void @Describe_circle
// CHECK: dispatch.base
// CHECK: call void @Describe
