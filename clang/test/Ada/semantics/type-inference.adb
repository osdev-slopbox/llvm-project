-- Test: type inference
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure type_inference is
    x : Integer := 5;
    y : Integer := x;
begin
    null;
end type_inference;
