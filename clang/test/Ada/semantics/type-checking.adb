-- Test: type checking
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure type_checking is
    x : Integer := 5;
    y : Float := 3.14;
    z : Boolean := x > 0;
begin
    null;
end type_checking;
