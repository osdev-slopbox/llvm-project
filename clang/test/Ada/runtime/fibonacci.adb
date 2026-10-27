-- Test: fibonacci sequence
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure fibonacci is
    function fib(n : Integer) return Integer is
        a : Integer := 0;
        b : Integer := 1;
        i : Integer := 0;
        temp : Integer;
    begin
        if n <= 0 then
            return 0;
        end if;
        if n = 1 then
            return 1;
        end if;
        while i < n - 1 loop
            temp := a + b;
            a := b;
            b := temp;
            i := i + 1;
        end loop;
        return b;
    end fib;
begin
    print_int(fib(10));
    new_line;
end fibonacci;

-- CHECK: 55
