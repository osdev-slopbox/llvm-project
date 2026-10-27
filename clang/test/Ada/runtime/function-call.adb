-- Test: function call
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure function_call is
    function double(x : Integer) return Integer is
    begin
        return x * 2;
    end double;
begin
    print_int(double(5));
    new_line;
end function_call;

-- CHECK: 10
