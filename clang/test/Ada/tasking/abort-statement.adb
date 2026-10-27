-- Test: abort statement (P10)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure test_abort is
    task T is
    end T;

    task body T is
    begin
        loop
            null;
        end loop;
    end T;

    completed : Integer := 0;
begin
    -- Abort the task
    abort T;
    completed := 1;
end test_abort;