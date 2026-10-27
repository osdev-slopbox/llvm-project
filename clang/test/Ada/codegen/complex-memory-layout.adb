-- Test: complex memory layout
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_memory_layout is
    type Rec is record
        a : Integer;
        b : Character;
        c : Float;
    end record;

    type Arr is array (1..100) of Rec;
    obj : Arr;
begin
    null;
end complex_memory_layout;
