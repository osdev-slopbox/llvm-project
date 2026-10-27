-- Test: protected function
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure protected_function is
    protected P is
        function Get_Value return Integer;
    private
        value : Integer := 0;
    end P;

    protected body P is
        function Get_Value return Integer is
        begin
            return value;
        end Get_Value;
    end P;
begin
    null;
end protected_function;
