-- Test: complex protected object
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_protected_object is
    protected Counter is
        procedure Increment;
        procedure Decrement;
        function Get return Integer;
    private
        value : Integer := 0;
    end Counter;

    protected body Counter is
        procedure Increment is
        begin
            value := value + 1;
        end Increment;

        procedure Decrement is
        begin
            value := value - 1;
        end Decrement;

        function Get return Integer is
        begin
            return value;
        end Get;
    end Counter;
begin
    Counter.Increment;
    Counter.Increment;
    Counter.Decrement;
end complex_protected_object;
