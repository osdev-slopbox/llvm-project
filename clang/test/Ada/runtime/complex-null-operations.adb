-- Test: null operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_null_operations is
begin
    print_int(1);
    new_line;
end complex_null_operations;

-- CHECK: 1
