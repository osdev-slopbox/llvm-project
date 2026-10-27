-- Test: for loop reverse
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure for_loop_reverse is
    i : Integer;
begin
    for i in reverse 1..10 loop
        null;
    end loop;
end for_loop_reverse;
