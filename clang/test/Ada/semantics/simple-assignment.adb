// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Simple assignment semantic check
procedure Simple_Assignment is
    X : Integer := 10;
    Y : Integer;
begin
    Y := X + 5;
end Simple_Assignment;
