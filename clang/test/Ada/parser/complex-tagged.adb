-- Test: complex tagged type
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_tagged is
    type Shape is tagged record
        color : String(1..10);
    end record;

    type Circle is new Shape with record
        radius : Integer;
    end record;

    type Rectangle is new Shape with record
        width : Integer;
        height : Integer;
    end record;
begin
    null;
end complex_tagged;
