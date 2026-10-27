-- Test: assertion handling
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_assertion_handling is
    result : Integer := 0;
begin
    if 5 = 5 then
        result := 1;
    end if;
    print_int(result);
    new_line;
end complex_assertion_handling;

-- CHECK: 1
