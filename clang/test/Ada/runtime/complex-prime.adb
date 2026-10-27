-- Test: prime number check
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_prime is
    function is_prime(n : Integer) return Integer is
        i : Integer := 2;
    begin
        if n <= 1 then
            return 0;
        end if;
        while i * i <= n loop
            if n mod i = 0 then
                return 0;
            end if;
            i := i + 1;
        end loop;
        return 1;
    end is_prime;
begin
    print_int(is_prime(17));
    new_line;
end complex_prime;

-- CHECK: 1
