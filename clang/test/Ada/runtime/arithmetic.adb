-- Test: arithmetic operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure arithmetic is
    result : Integer;
begin
    result := 2 + 3;
    print_int(result);
    new_line;
    result := 10 - 4;
    print_int(result);
    new_line;
    result := 5 * 6;
    print_int(result);
    new_line;
    result := 20 / 4;
    print_int(result);
    new_line;
end arithmetic;

-- CHECK: 5
-- CHECK: 6
-- CHECK: 30
-- CHECK: 5
