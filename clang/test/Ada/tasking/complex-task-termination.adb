-- Test: complex task termination
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_task_termination is
    done : Boolean := false;

    task T is
        entry Finish;
    end T;

    task body T is
    begin
        loop
            select
                accept Finish do
                    null;
                end Finish;
                exit;
            or
                null;
            end select;
        end loop;
    end T;
begin
    T.Finish;
end complex_task_termination;
