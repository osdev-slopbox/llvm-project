-- Test: block statement
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure block_statement is
begin
    declare
        local_var : Integer := 42;
    begin
        null;
    end;
end block_statement;
