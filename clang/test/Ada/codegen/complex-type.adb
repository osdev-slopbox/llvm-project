-- Test: complex type
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_type is
    type Matrix is array (1..10, 1..10) of Integer;
    m : Matrix;
    i, j : Integer;
begin
    for i in 1..10 loop
        for j in 1..10 loop
            m(i, j) := i * j;
        end loop;
    end loop;
    null;
end complex_type;
