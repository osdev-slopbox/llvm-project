// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: sh -c 'timeout 5 %t_prog; test $? = 0'

// Contracts pass: all preconditions and postconditions are satisfied.
// The function returns normally and Main exits with 0.

procedure Main is
   function Add(A : Integer; B : Integer) return Integer
     with Pre => A >= 0 and B >= 0, Post => Result >= A is
   begin
      return A + B;
   end Add;

   Result : Integer;
begin
   Result := Add(5, 10);
end Main;
