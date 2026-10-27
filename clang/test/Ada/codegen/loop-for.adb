-- Test: for loop with range
-- RUN: %clang_ada -a2012 -c %s -o /dev/null

procedure loop_for is
    i : Integer;
    sum : Integer := 0;
begin
    for i in 1..5 loop
        sum := sum + i;
    end loop;
    Put(sum);
    New_Line;
end loop_for;
