-- Test: float operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_float_operations is
begin
    print_int(4);
    new_line;
end complex_float_operations;

-- CHECK: 4
