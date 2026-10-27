-- Test: complex requeue
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_requeue is
    task T is
        entry E1;
        entry E2;
    end T;

    task body T is
    begin
        loop
            accept E1 do
                requeue E2;
            end E1;
        end loop;
    end T;
begin
    null;
end complex_requeue;
