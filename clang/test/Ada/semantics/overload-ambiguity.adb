-- Test: overload ambiguity
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure overload_ambiguity is
    function add(a : Integer; b : Integer) return Integer is
    begin
        return a + b;
    end add;

    function add(a : Float; b : Float) return Float is
    begin
        return a + b;
    end add;

    result : Integer;
begin
    result := add(1, 2);
end overload_ambiguity;
