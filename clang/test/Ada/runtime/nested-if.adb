-- Test: nested if statements
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure nested_if is
    x : Integer := 15;
    result : Integer := 0;
begin
    if x < 10 then
        result := 1;
    end if;
    if result = 0 and x < 20 then
        result := 2;
    end if;
    if result = 0 then
        result := 3;
    end if;
    print_int(result);
    new_line;
end nested_if;

-- CHECK: 2
