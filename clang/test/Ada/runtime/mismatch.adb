// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: sh -c '%t_prog 2> %t_err; test $? = 134'
// RUN: FileCheck %s --check-prefix=ERR --input-file %t_err

-- Handler catches the wrong name: must re-raise (propagate to main -> abort).
procedure Mismatch is
begin
   try
      raise Constraint_Error;
   exception
      when Storage_Error is
         null;
   end try;
end Mismatch;

ERR: ada: unhandled exception: Constraint_Error
