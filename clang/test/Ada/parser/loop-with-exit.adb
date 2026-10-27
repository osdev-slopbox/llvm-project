-- Test: loop with exit
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure loop_with_exit is
    x : Integer := 0;
begin
    loop
        exit when x >= 10;
        x := x + 1;
    end loop;
end loop_with_exit;
