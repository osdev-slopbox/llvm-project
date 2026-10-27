-- Test: conditional expressions
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure conditional_expressions is
    x : Integer := 5;
    y : Integer := 10;
    result : Integer;
begin
    result := 1;
    if x > y then
        result := 0;
    else
        result := 1;
    end if;
end conditional_expressions;
