-- Test: complex timeout
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_timeout is
    task T is
        entry Work;
    end T;

    task body T is
    begin
        loop
            accept Work do
                null;
            end Work;
        end loop;
    end T;
begin
    select
        T.Work;
    or
        delay 1.0;
    end select;
end complex_timeout;
