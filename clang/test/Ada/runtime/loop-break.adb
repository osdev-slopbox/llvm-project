-- Test: loop break with exit
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure loop_break is
    i : Integer := 0;
    sum : Integer := 0;
begin
    loop
        exit when i >= 5;
        sum := sum + i;
        i := i + 1;
    end loop;
    print_int(sum);
    new_line;
end loop_break;

-- CHECK: 10
