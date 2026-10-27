-- Test: alignment
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure alignment is
    type Aligned is record
        x : Integer;
        y : Integer;
    end record with Alignment => 8;
    obj : Aligned;
begin
    null;
end alignment;
