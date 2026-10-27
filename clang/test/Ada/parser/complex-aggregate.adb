-- Test: complex aggregate
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_aggregate is
    type Person is record
        name : String(1..20);
        age : Integer;
    end record;
    p : Person := (name => "Alice", age => 30);
begin
    null;
end complex_aggregate;
