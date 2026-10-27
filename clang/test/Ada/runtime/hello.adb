// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog
// Test: Ada.Text_IO Put_Line equivalent via runtime

procedure Hello is
begin
   put_line("Hello, World!");
end Hello;

// CHECK: Hello, World!
