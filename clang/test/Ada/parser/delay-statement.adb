-- Test: delay statement
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure delay_statement is
begin
    delay 1.0;
end delay_statement;
