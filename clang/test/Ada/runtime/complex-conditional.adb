-- Test: complex conditional
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_conditional is
    result : Integer := 0;
begin
    if 5 > 3 and 10 > 5 then
        result := 1;
    end if;
    print_int(result);
    new_line;
end complex_conditional;

-- CHECK: 1
