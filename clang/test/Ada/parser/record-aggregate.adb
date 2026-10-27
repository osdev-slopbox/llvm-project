-- Test: record aggregate initialization
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure record_aggregate is
    type Point is record
        x : Integer;
        y : Integer;
    end record;
    p : Point := (x => 1, y => 2);
begin
    null;
end record_aggregate;
