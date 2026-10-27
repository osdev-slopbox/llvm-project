-- Test: for loop
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure for_loop is
    i : Integer;
    sum : Integer := 0;
begin
    for i in 1..10 loop
        sum := sum + i;
    end loop;
end for_loop;
