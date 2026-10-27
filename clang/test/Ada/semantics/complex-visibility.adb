-- Test: complex visibility
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_visibility is
    x : Integer := 5;

    procedure inner is
        y : Integer := 10;
    begin
        null;
    end inner;
begin
    null;
end complex_visibility;
