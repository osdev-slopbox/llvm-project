with Ada.Text_IO;
with Ada.Integer_Text_IO;
use Ada.Text_IO;
use Ada.Integer_Text_IO;

procedure Fibonacci is
    procedure Fib(N : Integer; A, B : in out Integer) is
    begin
        if N > 1 then
            declare
                Tmp : Integer := A + B;
            begin
                A := B;
                B := Tmp;
                Fib(N - 1, A, B);
            end;
        end if;
    end Fib;

    A : Integer := 0;
    B : Integer := 1;
    I : Integer := 0;
begin
    while I < 10 loop
        Print(A);
        Put(" ");
        declare
            Tmp : Integer := A + B;
        begin
            A := B;
            B := Tmp;
        end;
        I := I + 1;
    end loop;
    New_Line;
end Fibonacci;