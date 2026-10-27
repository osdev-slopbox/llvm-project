-- Test: type compatibility
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure type_compatibility is
    x : Integer := 5;
    y : Float := x;
begin
    null;
end type_compatibility;
