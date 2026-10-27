-- Test: loop invariants
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure loop_invariants is
    sum : Integer := 0;
    i : Integer;
begin
    for i in 1..100 loop
        sum := sum + i;
    end loop;
end loop_invariants;
