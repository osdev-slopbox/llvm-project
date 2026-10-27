// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog
// Test: Nested exception handling with re-raise

procedure Exception_Nested is
   procedure Inner is
   begin
      raise Inner_Error;
   exception
      when Inner_Error =>
         raise Outer_Error;
   end Inner;
begin
   Inner;
exception
   when Outer_Error =>
      put_line("Caught outer");
end Exception_Nested;

// CHECK: Caught outer
