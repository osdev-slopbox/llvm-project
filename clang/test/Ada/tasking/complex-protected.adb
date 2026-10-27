-- Test: complex protected object
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_protected is
    protected P is
        procedure Do_Work;
        function Get_Value return Integer;
        entry Wait for Value;
    private
        value : Integer := 0;
    end P;

    protected body P is
        procedure Do_Work is
        begin
            value := value + 1;
        end Do_Work;

        function Get_Value return Integer is
        begin
            return value;
        end Get_Value;

        entry Wait for Value when (value > 0) is
        begin
            null;
        end Wait for Value;
    end P;
begin
    null;
end complex_protected;
