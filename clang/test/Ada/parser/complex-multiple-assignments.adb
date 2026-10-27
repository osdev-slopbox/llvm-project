-- Test: multiple assignments
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_multiple_assignments is
    a, b, c : Integer;
begin
    a := 1;
    b := 2;
    c := 3;
end complex_multiple_assignments;
