-- Test: task synchronization
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_task_synchronization is
begin
    print_int(1);
    new_line;
end complex_task_synchronization;

-- CHECK: 1
