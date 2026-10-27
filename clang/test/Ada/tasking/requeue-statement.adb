-- Test: requeue statement (P10)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure test_requeue is
    task T is
        entry Process(X : Integer);
        entry Done;
    end T;

    task body T is
        Count : Integer := 0;
    begin
        loop
            select
                accept Process(X : Integer) do
                    Count := Count + 1;
                    if Count < 5 then
                        requeue Process;
                    end if;
                end;
            or
                accept Done;
                exit;
            end select;
        end loop;
    end T;
begin
    null;
end test_requeue;