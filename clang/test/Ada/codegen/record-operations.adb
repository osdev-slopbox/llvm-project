-- Test: record operations
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure record_operations is
    type Point is record
        x : Integer;
        y : Integer;
    end record;
    p : Point;
begin
    p.x := 1;
    p.y := 2;
    null;
end record_operations;
