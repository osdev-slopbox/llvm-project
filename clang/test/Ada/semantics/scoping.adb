-- Test: scoping rules
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure scoping is
    x : Integer := 1;
begin
    declare
        x : Integer := 2;
    begin
        null;
    end;
    null;
end scoping;
