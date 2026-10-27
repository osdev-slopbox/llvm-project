-- Test: subtraction
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_subtract is
begin
    print_int(7);
    new_line;
end complex_subtract;

-- CHECK: 7
