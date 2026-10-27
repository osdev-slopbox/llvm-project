-- Test: derived types
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure derived_types is
    type Base is tagged record
        x : Integer;
    end record;
    type Derived is new Base with record
        y : Integer;
    end record;
    d : Derived;
begin
    null;
end derived_types;
