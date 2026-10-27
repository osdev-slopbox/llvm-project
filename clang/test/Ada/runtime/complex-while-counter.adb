-- Test: while loop counter
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_while_counter is
    count : Integer := 0;
    i : Integer := 0;
begin
    while i < 5 loop
        count := count + 1;
        i := i + 1;
    end loop;
    print_int(count);
    new_line;
end complex_while_counter;

-- CHECK: 5
