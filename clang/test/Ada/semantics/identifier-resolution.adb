-- Test: identifier resolution
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure identifier_resolution is
    x : Integer := 5;
    y : Integer := x;
begin
    null;
end identifier_resolution;
