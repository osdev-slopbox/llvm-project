-- Test: multiplication
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_multiply is
begin
    print_int(48);
    new_line;
end complex_multiply;

-- CHECK: 48
