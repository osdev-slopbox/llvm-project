// RUN: env ADA_TEST_VALUE=world %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: env ADA_TEST_VALUE=world timeout 5 %t_prog | FileCheck %s
// Test: Ada.Calendar, Ada.Strings.Fixed, Ada.Environment_Variables minima,
// Test: and overloaded Text_IO.Put for Integer/Float.

with Ada.Text_IO; use Ada.Text_IO;
with Ada.Calendar;
with Ada.Strings.Fixed;
with Ada.Environment_Variables;

procedure Stdlib is
   N : Integer := 42;
   F : Float := 2.5;
   S : String := "hello";
begin
   Put(N);
   New_Line;
   Put(F);
   New_Line;
   Put_Line(S);
   Put(Ada.Strings.Fixed.Length(S));
   New_Line;
   Put_Line(Ada.Environment_Variables.Value("ADA_TEST_VALUE"));
   if Ada.Environment_Variables.Exists("ADA_TEST_VALUE") then
      Put_Line("exists");
   end if;
   if Ada.Calendar.Clock > 0.0 then
      Put_Line("clock");
   end if;
end Stdlib;

// CHECK: 42
// CHECK-NEXT: 2.500000
// CHECK-NEXT: hello
// CHECK-NEXT: 5
// CHECK-NEXT: world
// CHECK-NEXT: exists
// CHECK-NEXT: clock
