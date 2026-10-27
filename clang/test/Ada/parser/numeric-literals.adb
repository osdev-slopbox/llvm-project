-- Test: numeric literal forms
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure numeric_literals is
    x : Integer := 123;
    y : Integer := 16#FF#;
    z : Integer := 2#1010#;
    a : Float := 3.14;
    b : Float := 1.5e-2;
    c : Float := 1.0E+5;
begin
    null;
end numeric_literals;
