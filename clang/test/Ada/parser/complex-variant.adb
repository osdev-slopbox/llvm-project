-- Test: complex variant record
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_variant is
    type Message_Type is (Text, Numeric, Binary);
    type Message (Type : Message_Type := Text) is record
        case Type is
            when Text =>
                Text_Data : String(1..100);
            when Numeric =>
                Numeric_Data : Integer;
            when Binary =>
                Binary_Data : array (1..10) of Character;
        end case;
    end record;
begin
    null;
end complex_variant;
