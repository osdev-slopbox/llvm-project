-- Test: complex division
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_division is
    result : Integer := 0;
begin
    result := 100 / 5;
    print_int(result);
    new_line;
end complex_division;

-- CHECK: 20
