-- Test: character and string literals
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure char_literals is
    ch : Character := 'A';
    ch2 : Character := '''';
    s : String := "Hello""World";
    empty : String := "";
begin
    null;
end char_literals;
