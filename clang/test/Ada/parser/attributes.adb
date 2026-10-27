-- Test: attribute references
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure attributes is
    arr : array (1..10) of Integer;
    x : Integer := arr'Length;
    y : Integer := arr'First;
    z : Integer := arr'Last;
begin
    null;
end attributes;
