-- Test: discriminated union
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_union is
    type Shape_Kind is (Circle, Rectangle);
    type Shape (Kind : Shape_Kind) is record
        case Kind is
            when Circle =>
                radius : Integer;
            when Rectangle =>
                width : Integer;
                height : Integer;
        end case;
    end record;
begin
    null;
end complex_union;
