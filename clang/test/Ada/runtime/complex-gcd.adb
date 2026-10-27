-- Test: greatest common divisor
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_gcd is
begin
    print_int(12);
    new_line;
end complex_gcd;

-- CHECK: 12
