-- Test: character operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_character_operations is
begin
    print_int(1);
    new_line;
end complex_character_operations;

-- CHECK: 1
