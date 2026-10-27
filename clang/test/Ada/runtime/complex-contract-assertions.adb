-- Test: contract assertions
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_contract_assertions is
    function factorial(n : Integer) return Integer
        with Pre => n >= 0,
             Post => factorial'Result >= 1
    is
    begin
        if n <= 1 then
            return 1;
        end if;
        return n * factorial(n - 1);
    end factorial;
begin
    print_int(factorial(5));
    new_line;
end complex_contract_assertions;

-- CHECK: 120
