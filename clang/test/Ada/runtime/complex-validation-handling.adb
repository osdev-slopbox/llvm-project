-- Test: validation handling
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_validation_handling is
    result : Integer := 0;
begin
    if 5 > 0 then
        result := 1;
    end if;
    print_int(result);
    new_line;
end complex_validation_handling;

-- CHECK: 1
