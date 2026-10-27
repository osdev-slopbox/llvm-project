-- RM 4.5 Arithmetic operations
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: 15
-- CHECK: 3
-- CHECK: 50
-- CHECK: 5
-- CHECK: 10

procedure Arithmetic is
    use Ada.Integer_Text_IO;
    use Ada.Text_IO;
begin
    print_int(10 + 5);
    New_Line;
    print_int(10 - 7);
    New_Line;
    print_int(10 * 5);
    New_Line;
    print_int(50 / 10);
    New_Line;
    print_int(10 * 10 / 10);
    New_Line;
end Arithmetic;