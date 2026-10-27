-- Test: constant declaration
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure const_declaration is
    const MAX : Integer := 100;
    const PI : Float := 3.14159;
    x : Integer := MAX;
begin
    null;
end const_declaration;
