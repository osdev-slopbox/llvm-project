// RUN: %clang_ada -a2012 -fsyntax-only %s

procedure PragmaTest is
    pragma Inline(PragmaTest);
    X : Integer := 42;
begin
    null;
end PragmaTest;
