-- Test: negative numbers
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure negative_numbers is
    result : Integer;
begin
    result := -5;
    print_int(result);
    new_line;
end negative_numbers;

-- CHECK: -5
