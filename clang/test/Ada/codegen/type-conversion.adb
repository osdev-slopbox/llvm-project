-- Test: type conversion
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure type_conversion is
    x : Integer := 42;
    y : Float;
    z : Integer;
begin
    y := Float(x);
    z := Integer(y);
    null;
end type_conversion;
