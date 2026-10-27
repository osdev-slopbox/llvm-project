with Ada.Text_IO;
use Ada.Text_IO;

procedure Contracts is
    function Max(A, B : Integer) return Integer
        with Pre => True,
             Post => Max'Result >= A and Max'Result >= B
    is
    begin
        if A > B then
            return A;
        else
            return B;
        end if;
    end Max;

begin
    Put("Max(5, 10) = ");
    Put(Integer'Image(Max(5, 10)));
    New_Line;
    Put("Max(20, 15) = ");
    Put(Integer'Image(Max(20, 15)));
    New_Line;
end Contracts;