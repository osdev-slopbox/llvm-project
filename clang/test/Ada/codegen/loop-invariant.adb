-- Test: loop invariant code motion
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure loop_invariant is
    sum : Integer := 0;
    i : Integer;
    multiplier : Integer := 2;
begin
    for i in 1..100 loop
        sum := sum + i * multiplier;
    end loop;
    null;
end loop_invariant;
