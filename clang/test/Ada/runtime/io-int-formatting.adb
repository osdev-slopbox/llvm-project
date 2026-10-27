-- Test: integer formatting edge cases (P10 GNAT compatibility)
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: 0
-- CHECK: -0
-- CHECK: 123
-- CHECK: -123
-- CHECK: 1000000
-- CHECK: -1000000

procedure io_int_formatting is
    use Ada.Integer_Text_IO;
    use Ada.Text_IO;
begin
    Put(0);
    New_Line;
    Put(-0);
    New_Line;
    Put(123);
    New_Line;
    Put(-123);
    New_Line;
    Put(1000000);
    New_Line;
    Put(-1000000);
    New_Line;
end io_int_formatting;