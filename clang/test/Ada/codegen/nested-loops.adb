-- Test: nested loops
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure nested_loops is
    i, j : Integer;
    result : Integer := 0;
begin
    for i in 1..5 loop
        for j in 1..5 loop
            result := result + i * j;
        end loop;
    end loop;
    Put(result);
    New_Line;
end nested_loops;
