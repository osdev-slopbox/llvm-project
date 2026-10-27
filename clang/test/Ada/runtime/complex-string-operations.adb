-- Test: string operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_string_operations is
begin
    put("Hello");
    new_line;
end complex_string_operations;

-- CHECK: Hello
