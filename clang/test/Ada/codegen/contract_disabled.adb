// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 -fno-ada-contracts -c %s -o %t.o -emit-llvm | FileCheck %s
// RUN: %clang_ada -a2012 -fno-ada-contracts -c %s -o %t.o -emit-llvm | opt -passes=verify -o /dev/null

procedure Main is
   function Add(A : Integer; B : Integer) return Integer
     with Pre => A >= 0 and B >= 0, Post => 'Result >= A is
   begin
      return A + B;
   end Add;
begin
   null;
end Main;

// With -fno-ada-contracts, contract checks are omitted. The function
// body contains no pre.fail or post.fail blocks.
// CHECK: define i32 @Add
// CHECK-NOT: pre.fail
// CHECK-NOT: post.fail
// CHECK-NOT: __ada_raise
// CHECK: ret
