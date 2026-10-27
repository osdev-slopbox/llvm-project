-- Test: conditional expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure cond_expr is
    x : Integer := 5;
    result : Integer;
begin
    result := if x > 0 then 1 else -1;
end cond_expr;
