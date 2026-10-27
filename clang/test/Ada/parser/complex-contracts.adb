-- Test: complex contracts
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_contracts is
    function factorial(n : Integer) return Integer
        with Pre => n >= 0,
             Post => factorial'Result > 0
    is
    begin
        if n <= 1 then
            return 1;
        end if;
        return n * factorial(n - 1);
    end factorial;
begin
    null;
end complex_contracts;
