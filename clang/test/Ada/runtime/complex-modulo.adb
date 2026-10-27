-- Test: modulo operation
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_modulo is
begin
    print_int(17);
    new_line;
end complex_modulo;

-- CHECK: 17
