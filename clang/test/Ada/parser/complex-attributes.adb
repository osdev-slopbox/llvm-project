-- Test: complex attributes
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_attributes is
    arr : array (1..10) of Integer;
    s : String := "hello";
    x : Integer := arr'Length;
    y : Integer := arr'First;
    z : Integer := arr'Last;
    w : Integer := s'Length;
begin
    null;
end complex_attributes;
