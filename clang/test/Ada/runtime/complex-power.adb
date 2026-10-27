-- Test: power operation
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_power is
begin
    print_int(256);
    new_line;
end complex_power;

-- CHECK: 256
