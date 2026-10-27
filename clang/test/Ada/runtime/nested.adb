// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog

-- Nested try: inner handler catches; verify handler stack pop ordering
-- by raising again after the first try completes.
procedure Nested is
   Count : Integer := 0;
begin
   try
      raise Constraint_Error;
   exception
      when Constraint_Error is
         Count := Count + 1;
   end try;
   -- Second try must work too (handler pushed/popped correctly).
   try
      raise Program_Error;
   exception
      when Program_Error is
         Count := Count + 1;
   end try;
   -- Third try: "when others" catches anything, including Storage_Error.
   try
      raise Storage_Error;
   exception
      when others is
         Count := Count + 1;
   end try;
end Nested;
