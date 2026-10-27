-- Test: complex task communication
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_task_communication is
    task Producer is
        entry Request(value : out Integer);
    end Producer;

    task Consumer is
        entry Process(value : in Integer);
    end Consumer;

    task body Producer is
        value : Integer := 0;
    begin
        loop
            accept Request(val : out Integer) do
                value := value + 1;
                val := value;
            end Request;
        end loop;
    end Producer;

    task body Consumer is
    begin
        loop
            accept Process(val : in Integer) do
                null;
            end Process;
        end loop;
    end Consumer;
begin
    null;
end complex_task_communication;
