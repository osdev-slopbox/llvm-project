-- Test: overloading resolution
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure overloading_resolution is
    function add(a : Integer; b : Integer) return Integer is
    begin
        return a + b;
    end add;

    function add(a : Float; b : Float) return Float is
    begin
        return a + b;
    end add;

    result_int : Integer;
    result_float : Float;
begin
    result_int := add(1, 2);
    result_float := add(1.0, 2.0);
end overloading_resolution;
