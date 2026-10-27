-- Test: visibility and scope
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure visibility_scope is
    x : Integer := 5;
begin
    declare
        y : Integer := 10;
    begin
        null;
    end;
    null;
end visibility_scope;
