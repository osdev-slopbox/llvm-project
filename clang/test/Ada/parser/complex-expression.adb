-- Test: complex expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_expression is
    a, b, c, d : Integer := 5;
    result : Integer;
begin
    result := a * b + c - d;
end complex_expression;
