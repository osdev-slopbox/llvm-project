-- Test: complex loop
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_loop is
    sum : Integer := 0;
    i, j : Integer;
begin
    for i in 1..10 loop
        for j in 1..10 loop
            sum := sum + i * j;
        end loop;
    end loop;
    null;
end complex_loop;
