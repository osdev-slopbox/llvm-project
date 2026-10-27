-- Test: while true loop
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure while_true_loop is
    x : Integer := 0;
begin
    while true loop
        exit when x >= 10;
        x := x + 1;
    end loop;
end while_true_loop;
