-- Test: complex expression evaluation
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_expressions is
    a, b, c, d : Integer := 5;
    x : Integer;
    y : Float := 2.0;
    z : Float;
begin
    x := a * b + c - d;
    z := a / b + c * d;
    if a < b and c > d then
        x := 1;
    end if;
    null;
end complex_expressions;
