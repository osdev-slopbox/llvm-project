-- Test: complex arithmetic
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_arithmetic is
begin
    print_int(11);
    new_line;
end complex_arithmetic;

-- CHECK: 11
