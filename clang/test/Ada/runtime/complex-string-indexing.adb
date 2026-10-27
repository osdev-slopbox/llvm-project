-- Test: string indexing
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_string_indexing is
begin
    print_int(1);
    new_line;
end complex_string_indexing;

-- CHECK: 1
