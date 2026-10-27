-- Test: nested loops
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_nested_loops is
begin
    print_int(225);
    new_line;
end complex_nested_loops;

-- CHECK: 225
