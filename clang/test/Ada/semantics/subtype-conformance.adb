-- Test: subtype conformance
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure subtype_conformance is
    subtype Positive_Integer is Integer range 1..100;
    x : Positive_Integer := 5;
begin
    null;
end subtype_conformance;
