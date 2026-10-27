-- Test: timed entry call
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure timed_entry is
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
        null;
    end select;
end timed_entry;
