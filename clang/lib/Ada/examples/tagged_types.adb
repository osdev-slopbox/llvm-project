with Ada.Text_IO;
use Ada.Text_IO;

procedure Tagged_Types is
    type Shape is abstract tagged null record;

    function Area(S : Shape) return Float is abstract;

    procedure Describe(S : Shape) is
    begin
        Put("Area: ");
        Put(Float'Image(Area(S)));
        New_Line;
    end Describe;

    type Circle is new Shape with record
        Radius : Float;
    end record;

    function Area(S : Circle) return Float is
    begin
        return 3.14159 * S.Radius * S.Radius;
    end Area;

    type Square is new Shape with record
        Side : Float;
    end record;

    function Area(S : Square) return Float is
    begin
        return S.Side * S.Side;
    end Area;

    C : Circle := (Radius => 5.0);
    Sq : Square := (Side => 4.0);
begin
    Describe(C);
    Describe(Sq);
end Tagged_Types;