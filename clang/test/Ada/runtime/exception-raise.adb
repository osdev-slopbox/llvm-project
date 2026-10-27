-- Test: exception raise and catch
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure exception_raise is
    caught : Integer := 0;
begin
    try
        raise Constraint_Error;
    exception
        when Constraint_Error =>
            caught := 1;
    end;
    print_int(caught);
    new_line;
end exception_raise;

-- CHECK: 1
