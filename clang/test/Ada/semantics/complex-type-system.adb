-- Test: complex type system
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_type_system is
    subtype Positive is Integer range 1..100;
    subtype Small_Positive is Integer range 1..10;

    x : Positive := 50;
    y : Small_Positive := 5;
    z : Integer := x + y;
begin
    null;
end complex_type_system;
