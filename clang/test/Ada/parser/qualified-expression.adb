-- Test: qualified expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure qualified_expression is
    x : Integer := Integer'(5);
begin
    null;
end qualified_expression;
