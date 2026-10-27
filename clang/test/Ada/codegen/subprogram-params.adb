-- Test: subprogram parameters and return values
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure subprogram_params is
    function add(a : Integer; b : Integer) return Integer is
    begin
        return a + b;
    end add;

    function multiply(a : Integer; b : Integer) return Integer is
    begin
        return a * b;
    end multiply;

    function max(a : Integer; b : Integer) return Integer is
    begin
        if a > b then
            return a;
        else
            return b;
        end if;
    end max;

    result : Integer;
begin
    result := add(5, 3);
    result := multiply(5, 3);
    result := max(5, 10);
    Put(result);
    New_Line;
end subprogram_params;
