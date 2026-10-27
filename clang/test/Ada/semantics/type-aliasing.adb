-- Test: type aliasing
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure type_aliasing is
    type My_Integer is new Integer;
    x : My_Integer := 5;
    y : Integer := 10;
begin
    null;
end type_aliasing;
