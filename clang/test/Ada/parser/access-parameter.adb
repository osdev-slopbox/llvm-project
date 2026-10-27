-- Test: access parameter
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure access_parameter is
    type IntPtr is access Integer;
    procedure set_value(p : IntPtr; value : Integer) is
    begin
        p.all := value;
    end set_value;
begin
    null;
end access_parameter;
