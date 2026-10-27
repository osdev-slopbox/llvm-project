-- Test: comprehensive final test
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_final_test is
begin
    print_int(42);
    new_line;
end complex_final_test;

-- CHECK: 42
