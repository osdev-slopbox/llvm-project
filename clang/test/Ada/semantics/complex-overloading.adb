-- Test: complex overloading
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_overloading is
    function add(a : Integer; b : Integer) return Integer is
    begin
        return a + b;
    end add;

    function add(a : Float; b : Float) return Float is
    begin
        return a + b;
    end add;

    function add(a : Integer; b : Float) return Float is
    begin
        return Float(a) + b;
    end add;

    result1 : Integer;
    result2 : Float;
    result3 : Float;
begin
    result1 := add(1, 2);
    result2 := add(1.0, 2.0);
    result3 := add(1, 2.0);
end complex_overloading;
