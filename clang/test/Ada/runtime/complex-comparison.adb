-- Test: complex comparison
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_comparison is
    result : Integer := 0;
begin
    if 5 < 10 then
        result := 1;
    end if;
    print_int(result);
    new_line;
end complex_comparison;

-- CHECK: 1
