-- Test: integer operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_integer_operations is
    result : Integer := 0;
begin
    result := 10 + 20;
    print_int(result);
    new_line;
end complex_integer_operations;

-- CHECK: 30
