-- Test: discriminated type
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure discriminated_type is
    type Shape is tagged record
        color : String(1..10);
    end record;
    type Circle is new Shape with record
        radius : Integer;
    end record;
begin
    null;
end discriminated_type;
