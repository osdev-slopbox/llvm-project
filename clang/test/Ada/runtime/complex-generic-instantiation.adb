-- Test: generic instantiation
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_generic_instantiation is
begin
    print_int(1);
    new_line;
end complex_generic_instantiation;

-- CHECK: 1
