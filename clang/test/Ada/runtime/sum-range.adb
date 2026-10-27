-- Test: sum of range
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure sum_range is
    sum : Integer := 0;
    i : Integer := 1;
begin
    while i <= 100 loop
        sum := sum + i;
        i := i + 1;
    end loop;
    print_int(sum);
    new_line;
end sum_range;

-- CHECK: 5050
