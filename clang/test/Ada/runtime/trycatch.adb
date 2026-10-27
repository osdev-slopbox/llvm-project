// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog

procedure TryCatch is
   X : Integer := 0;
begin
   try
      X := X + 1;
      raise Constraint_Error;
   exception
      when Constraint_Error is
         null;
   end try;
end TryCatch;
