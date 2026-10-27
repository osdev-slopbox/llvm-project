-- Test: array iteration
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_array_iteration is
begin
    print_int(55);
    new_line;
end complex_array_iteration;

-- CHECK: 55
