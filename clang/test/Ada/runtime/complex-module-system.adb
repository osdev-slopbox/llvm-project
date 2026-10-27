-- Test: module system
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_module_system is
begin
    print_int(1);
    new_line;
end complex_module_system;

-- CHECK: 1
