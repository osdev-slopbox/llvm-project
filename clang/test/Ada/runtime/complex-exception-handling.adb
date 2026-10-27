-- Test: exception handling
-- RUN: %clang_ada -a2012 %ada_link_flags %s -o %t
-- RUN: %t | FileCheck %s

procedure complex_exception_handling is
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
end complex_exception_handling;

-- CHECK: 1
