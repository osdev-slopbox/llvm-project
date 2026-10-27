// RUN: %clang_ada -a2012 -fsyntax-only %s

procedure AspectTest with Inline is
    X : Integer := 42;
begin
    null;
end AspectTest;
