with Ada.Text_IO;
use Ada.Text_IO;

procedure Exceptions is
    procedure Divide(A, B : Integer) is
        Result : Integer;
    begin
        if B = 0 then
            raise Constraint_Error with "division by zero";
        end if;
        Result := A / B;
        Put("Result: ");
        Put(Integer'Image(Result));
        New_Line;
    exception
        when Constraint_Error =>
            Put("Error: ");
            Put_Line("constraint violated");
    end Divide;

begin
    Divide(10, 2);
    Divide(10, 0);
end Exceptions;