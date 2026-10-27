// RUN: %clang_ada -a2012 -c %s -o %t.o

procedure Hello is
    X : Integer := 42;
begin
    null;
end Hello;