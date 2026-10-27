// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: sh -c '%t_prog 2> %t_err; test $? = 134'
// RUN: FileCheck %s --check-prefix=ERR --input-file %t_err

// Precondition violation: A is negative, but Pre requires A >= 0.
// The check raises Constraint_Error, which propagates unhandled.

procedure Main is
   function Add(A : Integer; B : Integer) return Integer
     with Pre => A >= 0 and B >= 0 is
   begin
      return A + B;
   end Add;
begin
   Add(-5, 10);
end Main;

ERR: ada: unhandled exception: Constraint_Error
