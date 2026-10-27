-- Test: access type operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_access_operations is
begin
    print_int(42);
    new_line;
end complex_access_operations;

-- CHECK: 42
