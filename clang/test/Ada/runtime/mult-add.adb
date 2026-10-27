-- Test: multiply and add
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure mult_add is
    result : Integer;
begin
    result := 6 * 7;
    print_int(result);
    new_line;
end mult_add;

-- CHECK: 42
