-- Test: type compatibility with derived types (P10)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure test_type_compatibility is
    type Base is record
        x : Integer;
    end record;

    type Derived is new Base with record
        y : Integer;
    end record;

    b : Base;
    d : Derived;
begin
    b := d;  -- Derived is assignable to Base (subtype conformance)
    null;
end test_type_compatibility;