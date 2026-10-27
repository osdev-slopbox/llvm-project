-- Test: controlled types
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_controlled_types is
begin
    print_int(42);
    new_line;
end complex_controlled_types;

-- CHECK: 42
