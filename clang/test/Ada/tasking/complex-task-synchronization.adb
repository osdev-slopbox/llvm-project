-- Test: complex task synchronization
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_task_synchronization is
    counter : Integer := 0;

    protected Mutex is
        procedure Increment;
        procedure Decrement;
    private
        count : Integer := 0;
    end Mutex;

    protected body Mutex is
        procedure Increment is
        begin
            count := count + 1;
        end Increment;

        procedure Decrement is
        begin
            count := count - 1;
        end Decrement;
    end Mutex;

    task Worker is
        entry Start;
        entry Stop;
    end Worker;

    task body Worker is
    begin
        accept Start;
        Mutex.Increment;
        accept Stop;
        Mutex.Decrement;
    end Worker;

    w1 : Worker;
    w2 : Worker;
begin
    w1.Start;
    w2.Start;
    w1.Stop;
    w2.Stop;
end complex_task_synchronization;
