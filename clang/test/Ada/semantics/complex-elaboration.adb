-- Test: elaboration order
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_elaboration is
    x : Integer := 5;
    y : Integer := x + 1;
begin
    null;
end complex_elaboration;
