-- Test: complex closure
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_closure is
    x : Integer := 10;
    function make_adder(y : Integer) return Integer is
    begin
        return x + y;
    end make_adder;
begin
    null;
end complex_closure;
