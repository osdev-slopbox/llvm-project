-- Test: declaration shadowing
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure declaration_shadowing is
    x : Integer := 1;
begin
    declare
        x : Integer := 2;
    begin
        null;
    end;
    null;
end declaration_shadowing;
