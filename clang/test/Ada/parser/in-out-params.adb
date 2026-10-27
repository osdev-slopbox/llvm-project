-- Test: in-out parameters
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure in_out_params is
    procedure swap(a : in out Integer; b : in out Integer) is
        temp : Integer;
    begin
        temp := a;
        a := b;
        b := temp;
    end swap;

    x : Integer := 1;
    y : Integer := 2;
begin
    swap(x, y);
end in_out_params;
