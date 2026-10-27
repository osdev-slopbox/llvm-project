-- Test: complex discriminant
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_discriminant is
    type Shape (Kind : Character) is record
        case Kind is
            when 'C' =>
                radius : Integer;
            when 'R' =>
                width : Integer;
                height : Integer;
            when others =>
                null;
        end case;
    end record;
begin
    null;
end complex_discriminant;
