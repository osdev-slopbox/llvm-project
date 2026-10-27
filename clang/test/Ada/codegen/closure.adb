-- Test: closure (nested function with free variables)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure closure is
    x : Integer := 10;
    function add_y(y : Integer) return Integer is
    begin
        return x + y;
    end add_y;
begin
    null;
end closure;
