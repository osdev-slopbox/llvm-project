// RUN: %clang_ada -a2012 -fsyntax-only %s

procedure MultiplePragmas is
    pragma Inline(MultiplePragmas);
    X : Integer := 42;
begin
    null;
end MultiplePragmas;
