-- Test: bitwise operations
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure bitwise_ops is
    a : Integer := 12;
    b : Integer := 10;
    result1 : Integer;
    result2 : Integer;
    result3 : Integer;
begin
    result1 := a and b;
    result2 := a or b;
    result3 := a xor b;
    null;
end bitwise_ops;
