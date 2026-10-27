-- Test: complex type inference
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_type_inference is
    x : Integer := 5;
    y : Float := 3.14;
    z : Boolean := x > 0;
begin
    null;
end complex_type_inference;
