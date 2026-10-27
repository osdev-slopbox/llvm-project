-- Test: string concatenation and operations
-- RUN: %clang_ada -a2012 -c %s -o /dev/null

procedure strings is
    s1 : String := "Hello";
    s2 : String := " World";
    s3 : String;
begin
    s3 := s1 & s2;
    Put_Line(s3);
end strings;
