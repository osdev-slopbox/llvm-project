-- Test: complex boolean
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_boolean is
    result : Integer := 0;
begin
    if true or false then
        result := 1;
    end if;
    print_int(result);
    new_line;
end complex_boolean;

-- CHECK: 1
