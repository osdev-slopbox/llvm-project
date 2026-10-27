-- Test: complex loop nesting
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_loop_nesting is
    i, j : Integer;
begin
    for i in 1..10 loop
        for j in 1..10 loop
            null;
        end loop;
    end loop;
end complex_loop_nesting;
