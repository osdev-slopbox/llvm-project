// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog

-- Subprogram-level exception handler (RM 8.6(4)).
procedure SubprogExc is
   X : Integer := 0;
begin
   X := X + 1;
   raise Constraint_Error;
exception
   when Constraint_Error is
      null;
end SubprogExc;
