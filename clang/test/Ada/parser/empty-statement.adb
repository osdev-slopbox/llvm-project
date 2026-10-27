-- Test: empty statement
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure empty_statement is
begin
    null;
end empty_statement;
