-- Test: tagged type operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_tagged_operations is
begin
    print_int(3);
    new_line;
end complex_tagged_operations;

-- CHECK: 3
