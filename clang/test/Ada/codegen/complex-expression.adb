-- Test: complex expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_expression is
    a, b, c, d, e : Integer := 5;
    result : Integer;
begin
    result := a * b + c - d / e;
    null;
end complex_expression;
