-- RM A.1.1(1) Hello World
-- RUN: %clang_ada -a2012 -o %t %s %ada_link_flags
-- RUN: %t
-- CHECK: Hello, World!

procedure Hello is
    use Ada.Text_IO;
begin
    Put_Line("Hello, World!");
end Hello;