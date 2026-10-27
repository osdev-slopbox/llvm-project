-- Test: complex type
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_type is
    type Matrix is array (1..10, 1..10) of Integer;
    m : Matrix;
begin
    null;
end complex_type;
