-- Test: IO operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_io_operations is
begin
    print_int(42);
    new_line;
end complex_io_operations;

-- CHECK: 42
