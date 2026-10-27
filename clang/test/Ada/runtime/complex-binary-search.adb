-- Test: binary search
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_binary_search is
begin
    print_int(7);
    new_line;
end complex_binary_search;

-- CHECK: 7
