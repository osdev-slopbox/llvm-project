-- Test: parameter passing
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure parameter_passing is
    function add(a : Integer; b : Integer) return Integer is
    begin
        return a + b;
    end add;
begin
    print_int(add(5, 3));
    new_line;
end parameter_passing;

-- CHECK: 8
