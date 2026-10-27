-- Test: complex aliasing
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_aliasing is
    x : Integer := 5;
    y : Integer renames x;
begin
    y := 10;
end complex_aliasing;
