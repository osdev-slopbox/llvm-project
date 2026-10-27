-- Test: aliasing analysis
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure aliasing is
    type Rec is record
        x : Integer;
        y : Integer;
    end record;
    a : Rec;
    b : Rec;
begin
    a.x := 1;
    a.y := 2;
    b := a;
    null;
end aliasing;
