-- Test: memory pool
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_memory_pool is
begin
    print_int(1);
    new_line;
end complex_memory_pool;

-- CHECK: 1
