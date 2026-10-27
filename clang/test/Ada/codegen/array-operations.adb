-- Test: array operations
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure array_operations is
    arr : array (1..10) of Integer;
    i : Integer;
begin
    for i in 1..10 loop
        arr(i) := i;
    end loop;
    null;
end array_operations;
