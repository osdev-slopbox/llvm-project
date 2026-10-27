-- Test: complex expression nesting
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_expression_nesting is
    a, b, c, d : Integer := 5;
    result : Integer;
begin
    result := (a + b) * (c - d);
end complex_expression_nesting;
