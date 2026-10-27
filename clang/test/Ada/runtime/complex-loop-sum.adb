-- Test: loop sum
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_loop_sum is
    sum : Integer := 0;
    i : Integer := 1;
begin
    while i <= 10 loop
        sum := sum + i;
        i := i + 1;
    end loop;
    print_int(sum);
    new_line;
end complex_loop_sum;

-- CHECK: 55
