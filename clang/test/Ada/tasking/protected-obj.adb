-- Test: protected object
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure protected_obj is
    protected Counter is
        procedure Increment;
        function Get return Integer;
    private
        value : Integer := 0;
    end Counter;

    protected body Counter is
        procedure Increment is
        begin
            value := value + 1;
        end Increment;
        function Get return Integer is
        begin
            return value;
        end Get;
    end Counter;
begin
    null;
end protected_obj;
