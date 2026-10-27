// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Subprogram overloading (RM 8.2)
procedure Overloading is
    function Max(A, B : Integer) return Integer is
    begin
        if A > B then
            return A;
        else
            return B;
        end if;
    end Max;
    
    function Max(A, B : Float) return Float is
    begin
        if A > B then
            return A;
        else
            return B;
        end if;
    end Max;
    
    X : Integer;
    Y : Float;
begin
    X := Max(5, 10);
    Y := Max(3.0, 2.5);
end Overloading;
