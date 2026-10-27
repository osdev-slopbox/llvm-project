-- Test: complex generic instantiation
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_generic_instantiation is
    generic
        type Element is private;
        function Less(a, b : Element) return Boolean;
    procedure Sort(arr : in out array of Element);

    type My_Integer is new Integer;
    function Less(a, b : My_Integer) return Boolean is
    begin
        return a < b;
    end Less;

    procedure Sort_Ints is new Sort(Element => My_Integer, Less => Less);
begin
    null;
end complex_generic_instantiation;
