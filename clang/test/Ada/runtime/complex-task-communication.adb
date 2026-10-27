-- Test: task communication
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_task_communication is
begin
    print_int(1);
    new_line;
end complex_task_communication;

-- CHECK: 1
