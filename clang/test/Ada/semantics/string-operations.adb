// RUN: %clang_ada -fsyntax-only -a2012 %s
-- String operations (RM 3.6)
procedure String_Operations is
    S1 : String := "Hello";
    S2 : String := "World";
    S3 : String(1..11);
begin
    S3 := S1 & " " & S2;
    null;
end String_Operations;
