-- Test: loop with exit when
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure loop_exit is
    x : Integer := 10;
    count : Integer := 0;
begin
    loop
        exit when x = 0;
        count := count + 1;
        x := x - 1;
    end loop;
    print_int(count);
    new_line;
end loop_exit;

-- CHECK: 10
