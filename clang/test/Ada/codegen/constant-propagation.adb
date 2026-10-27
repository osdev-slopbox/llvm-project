-- Test: constant propagation
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure constant_propagation is
    const PI : Float := 3.14159;
    const TWO : Integer := 2;
    result : Float;
begin
    result := PI * TWO;
    null;
end constant_propagation;
