-- Test: visibility rules
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure visibility_rules is
    x : Integer := 5;
    y : Integer := 10;
begin
    null;
end visibility_rules;
