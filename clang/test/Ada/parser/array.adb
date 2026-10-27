// RUN: %clang_ada -fsyntax-only %s

procedure Test_Array is
    Arr : array(1 .. 10) of Integer;
    Matrix : array(1 .. 5, 1 .. 5) of Real;
begin
    Arr(1) := 42;
    Matrix(1, 1) := 3.14;
end Test_Array;