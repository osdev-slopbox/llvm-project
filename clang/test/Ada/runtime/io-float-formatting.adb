-- Test: float formatting edge cases (P10 GNAT compatibility)
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: 0.0
-- CHECK: -0.0
-- CHECK: 1.5
-- CHECK: -1.5
-- CHECK: 3.14159
-- CHECK: 100.0

procedure io_float_formatting is
    use Ada.Float_Text_IO;
    use Ada.Text_IO;
begin
    Put(0.0);
    New_Line;
    Put(-0.0);
    New_Line;
    Put(1.5);
    New_Line;
    Put(-1.5);
    New_Line;
    Put(3.14159);
    New_Line;
    Put(100.0);
    New_Line;
end io_float_formatting;