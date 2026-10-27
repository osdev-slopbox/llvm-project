-- Test: string formatting edge cases (P10 GNAT compatibility)
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: 
-- CHECK: Hello
-- CHECK: 
-- CHECK: 
-- CHECK: Hello World

procedure io_string_formatting is
    use Ada.Text_IO;
begin
    Put("");
    New_Line;
    Put("Hello");
    New_Line;
    Put(" ");
    New_Line;
    Put("  ");
    New_Line;
    Put("Hello World");
    New_Line;
end io_string_formatting;