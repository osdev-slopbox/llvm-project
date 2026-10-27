-- Test: tail call optimization opportunity
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure tail_call is
    function factorial(n : Integer) return Integer is
    begin
        if n <= 1 then
            return 1;
        end if;
        return n * factorial(n - 1);
    end factorial;
begin
    null;
end tail_call;
