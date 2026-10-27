-- Test: string operations
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure string_ops is
    s1 : String := "Hello";
    s2 : String := " World";
    s3 : String;
begin
    s3 := s1 & s2;
    null;
end string_ops;
