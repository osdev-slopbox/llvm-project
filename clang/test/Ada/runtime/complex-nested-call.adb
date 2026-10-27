-- Test: nested function calls
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_nested_call is
    function double(x : Integer) return Integer is
    begin
        return x * 2;
    end double;

    function quad(x : Integer) return Integer is
    begin
        return double(double(x));
    end quad;
begin
    print_int(quad(3));
    new_line;
end complex_nested_call;

-- CHECK: 12
