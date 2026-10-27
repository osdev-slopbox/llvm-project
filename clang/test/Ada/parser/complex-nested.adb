-- Test: complex nested structures
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_nested is
    type Inner is record
        x : Integer;
    end record;

    type Outer is record
        inner : Inner;
        y : Integer;
    end record;

    obj : Outer := (inner => (x => 1), y => 2);
begin
    null;
end complex_nested;
