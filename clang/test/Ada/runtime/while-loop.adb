-- Test: while loop
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure while_loop is
    x : Integer := 1;
    product : Integer := 1;
begin
    while x <= 10 loop
        product := product * x;
        x := x + 1;
    end loop;
    print_int(product);
    new_line;
end while_loop;

-- CHECK: 3628800
