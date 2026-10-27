-- Test: complex recursive function
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_recursive is
    function fib(n : Integer) return Integer is
    begin
        if n <= 1 then
            return n;
        end if;
        return fib(n - 1) + fib(n - 2);
    end fib;
begin
    null;
end complex_recursive;
