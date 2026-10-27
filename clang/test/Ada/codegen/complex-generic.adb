-- Test: complex generic
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_generic is
    generic
        type Element is private;
        function Less(a, b : Element) return Boolean;
    procedure Sort(arr : in out array of Element);

    type My_Integer is new Integer;
    function Less(a, b : My_Integer) return Boolean is
    begin
        return a < b;
    end Less;

    procedure Sort_Ints is new Sort(My_Integer, Less);
    arr : array (1..10) of My_Integer;
begin
    null;
end complex_generic;
