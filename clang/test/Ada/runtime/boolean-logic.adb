-- Test: boolean logic
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure boolean_logic is
    result : Integer := 0;
begin
    if true and true then
        result := 1;
    end if;
    print_int(result);
    new_line;
end boolean_logic;

-- CHECK: 1
