// RUN: %clang_ada -fsyntax-only -a2012 %s
-- Record component access (RM 3.7)
procedure Record_Component is
    type Person is record
        Name : String(1..20);
        Age : Integer;
    end record;
    
    P : Person;
begin
    P.Name := "Alice";
    P.Age := 30;
    null;
end Record_Component;
