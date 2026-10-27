-- Test: string slicing
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_string_slicing is
begin
    put("Hello");
    new_line;
end complex_string_slicing;

-- CHECK: Hello
