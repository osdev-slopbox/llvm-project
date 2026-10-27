// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: sh -c '%t_prog 2> %t_err; test $? = 134'
// RUN: FileCheck %s --check-prefix=ERR --input-file %t_err

-- Unhandled exception propagates to main -> abort (exit 134)
procedure Unhandled is
begin
   raise Program_Error;
end Unhandled;

ERR: ada: unhandled exception: Program_Error
