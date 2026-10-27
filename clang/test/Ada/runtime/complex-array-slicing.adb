-- Test: array slicing
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_array_slicing is
begin
    print_int(5);
    new_line;
end complex_array_slicing;

-- CHECK: 5
