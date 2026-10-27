-- Test: logical expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure logical_expression is
    a, b : Boolean := true;
    result : Boolean;
begin
    result := a and b;
    result := a or b;
    result := not a;
end logical_expression;
