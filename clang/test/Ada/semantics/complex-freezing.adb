-- Test: type freezing
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_freezing is
    type My_Type is record
        x : Integer;
    end record;

    obj : My_Type;

    type Extended_Type is new My_Type with record
        y : Integer;
    end record;
begin
    null;
end complex_freezing;
