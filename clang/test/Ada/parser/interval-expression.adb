-- Test: interval expression
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure interval_expression is
    x : Integer := 5;
begin
    if x in 1..10 then
        null;
    end if;
end interval_expression;
