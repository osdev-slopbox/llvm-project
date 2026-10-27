-- Test: while loop
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure while_loop is
    x : Integer := 0;
begin
    while x < 10 loop
        x := x + 1;
    end loop;
end while_loop;
