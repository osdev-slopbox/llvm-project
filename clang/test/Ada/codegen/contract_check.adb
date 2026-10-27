// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 -c %s -o %t.o -emit-llvm | FileCheck %s
// RUN: %clang_ada -a2012 -c %s -o %t.o -emit-llvm | opt -passes=verify -o /dev/null

procedure Main is
   function Add(A : Integer; B : Integer) return Integer
     with Pre => A >= 0 and B >= 0, Post => 'Result >= A is
   begin
      return A + B;
   end Add;
begin
   null;
end Main;

// Contract checks are enabled by default. The Pre aspect generates
// a check before the body, and the Post aspect generates a check
// before the return.
// CHECK: define i32 @Add
// CHECK: alloca i32
// CHECK: pre.fail
// CHECK: call void @__ada_raise
// CHECK: pre.ok
// CHECK: post.fail
// CHECK: call void @__ada_raise
// CHECK: post.ok
