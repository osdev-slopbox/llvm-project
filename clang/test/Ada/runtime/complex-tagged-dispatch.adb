-- Test: tagged type dispatch
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_tagged_dispatch is
begin
    print_int(1);
    new_line;
end complex_tagged_dispatch;

-- CHECK: 1
