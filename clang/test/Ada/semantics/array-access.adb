// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Array access and slicing (RM 4.7)
procedure Array_Access is
    type Int_Array is array (1..10) of Integer;
    A : Int_Array;
begin
    A(1) := 10;
    A(5) := 50;
    null;
end Array_Access;
