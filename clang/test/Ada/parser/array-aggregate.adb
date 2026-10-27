-- Test: array aggregate
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure array_aggregate is
    arr : array (1..5) of Integer := (1, 2, 3, 4, 5);
begin
    null;
end array_aggregate;
