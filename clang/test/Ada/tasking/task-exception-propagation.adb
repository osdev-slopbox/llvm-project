-- Test: exception propagation through task boundaries (P10)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure test_task_exception is
    task T is
    end T;

    task body T is
    begin
        raise Constraint_Error;
    end T;

    caught : Integer := 0;
begin
    try
        -- Join the task (wait for it to complete)
        -- The exception should propagate here
        null;
    exception
        when Constraint_Error =>
            caught := 1;
    end;
end test_task_exception;