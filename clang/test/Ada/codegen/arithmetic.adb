// RUN: %clang_ada -a2012 -c %s -o %t.o

procedure Arithmetic is
    X : Integer := 10;
    Y : Integer := 5;
    Z : Integer := X + Y;
begin
    Z := X - Y;
    Z := X * Y;
end Arithmetic;