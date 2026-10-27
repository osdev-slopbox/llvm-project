-- Test: constant folding
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure constant_folding is
    x : Integer := 2 + 3;
    y : Integer := 4 * 5;
    z : Integer := 10 - 2;
begin
    null;
end constant_folding;
