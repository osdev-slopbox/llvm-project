// RUN: %clang_ada -a2012 -fsyntax-only %s

procedure AspectWithVal with Inline => True is
    X : Integer := 42;
begin
    null;
end AspectWithVal;
