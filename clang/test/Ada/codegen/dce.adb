-- Test: dead code elimination opportunity
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure dce is
    x : Integer := 5;
    y : Integer := 10;
    result : Integer;
begin
    result := x + y;
    null;
end dce;
