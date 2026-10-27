// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog | FileCheck %s
// Test: Ada.Text_IO and Ada.Integer_Text_IO Put equivalents via runtime

procedure IO is
   X : Integer := 42;
   Y : Float := 3.14;
begin
   put("Hello");
   new_line;
   put("World");
   new_line;
   print_int(X);
   new_line;
   print_float(Y);
   new_line;
   print_int_new_line(100);
   print_float_new_line(2.71);
end IO;

// CHECK: Hello
// CHECK-NEXT: World
// CHECK-NEXT: 42
// CHECK-NEXT: 3.14
// CHECK-NEXT: 100
// CHECK-NEXT: 2.71
