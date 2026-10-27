-- Test: inline function
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure inline_function is
    function square(x : Integer) return Integer with Inline is
    begin
        return x * x;
    end square;
begin
    null;
end inline_function;
