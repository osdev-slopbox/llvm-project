-- Test: range expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure range_expression is
    arr : array (1..10) of Integer;
    i : Integer;
begin
    for i in 1..10 loop
        arr(i) := i;
    end loop;
end range_expression;
