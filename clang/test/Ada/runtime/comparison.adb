-- Test: comparison operations
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure comparison is
    x : Integer := 5;
    y : Integer := 10;
    result : Integer := 0;
begin
    if x < y then
        result := 1;
    end if;
    print_int(result);
    new_line;
end comparison;

-- CHECK: 1
