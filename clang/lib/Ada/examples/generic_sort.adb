with Ada.Text_IO;
with Ada.Integer_Text_IO;
use Ada.Text_IO;
use Ada.Integer_Text_IO;

generic
    type Element is private;
    type Index_Type is range <>;
    type Array_Type is array (Index_Type range <>) of Element;
    with function "<" (Left, Right : Element) return Boolean is <>;
procedure Bubble_Sort(X : in out Array_Type) is
    procedure Swap(A, B : in out Element) is
        Tmp : Element := A;
    begin
        A := B;
        B := Tmp;
    end Swap;

    Last : Index_Type := X'Last;
begin
    while Last > X'First loop
        declare
            I : Index_Type := X'First;
        begin
            while I + 1 <= Last loop
                if X(I + 1) < X(I) then
                    Swap(X(I), X(I + 1));
                end if;
                I := I + 1;
            end loop;
        end;
        Last := Last - 1;
    end loop;
end Bubble_Sort;

procedure Test_Sort is
    package Int_Sort is new Bubble_Sort(
        Element => Integer,
        Index_Type => Integer range 1..5,
        Array_Type => array (Integer range 1..5) of Integer);

    Arr : array (1..5) of Integer := (5, 3, 1, 4, 2);
    I : Integer := 1;
begin
    Int_Sort(Arr);
    while I <= 5 loop
        Print(Arr(I));
        Put(" ");
        I := I + 1;
    end loop;
    New_Line;
end Test_Sort;