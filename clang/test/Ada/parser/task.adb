// RUN: %clang_ada -fsyntax-only %s

procedure Test_Task is
    task Producer is
        entry Start;
        entry Get(Value : out Integer);
    end task;

    task body Producer is
        Count : Integer := 0;
    begin
        loop
            accept Start do
            end Start;
            exit;
        end loop;
    end Producer;
begin
    null;
end Test_Task;