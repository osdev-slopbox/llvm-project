-- Test: loop optimization opportunity
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure loop_optimization is
    sum : Integer := 0;
    i : Integer;
begin
    for i in 1..1000 loop
        sum := sum + i;
    end loop;
    null;
end loop_optimization;
