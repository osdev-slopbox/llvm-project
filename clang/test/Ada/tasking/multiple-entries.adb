-- Test: task with multiple entries
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure multiple_entries is
    task T is
        entry E1;
        entry E2;
        entry E3;
    end T;

    task body T is
    begin
        loop
            select
                accept E1 do
                    null;
                end E1;
            or
                accept E2 do
                    null;
                end E2;
            or
                accept E3 do
                    null;
                end E3;
            end select;
        end loop;
    end T;
begin
    null;
end multiple_entries;
