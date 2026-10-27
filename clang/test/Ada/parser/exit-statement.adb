-- Test: exit statement
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure exit_statement is
    x : Integer := 0;
begin
    loop
        exit when x >= 10;
        x := x + 1;
    end loop;
end exit_statement;
