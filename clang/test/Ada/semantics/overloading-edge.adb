-- Test: overloading edge cases
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure overloading_edge is
    function foo(x : Integer) return Integer is
    begin
        return x * 2;
    end foo;

    function foo(x : Float) return Float is
    begin
        return x * 2.0;
    end foo;

    result : Integer;
begin
    result := foo(5);
end overloading_edge;
