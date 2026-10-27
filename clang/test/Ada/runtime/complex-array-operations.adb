-- Test: array operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_array_operations is
begin
    print_int(55);
    new_line;
end complex_array_operations;

-- CHECK: 55
