-- Test: nested function
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure nested_function is
    function outer(x : Integer) return Integer is
        function inner(y : Integer) return Integer is
        begin
            return y * 2;
        end inner;
    begin
        return inner(x) + 1;
    end outer;
begin
    null;
end nested_function;
