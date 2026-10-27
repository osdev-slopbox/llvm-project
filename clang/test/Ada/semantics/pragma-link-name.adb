-- Test: Link_Name pragma (GNAT compatibility)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure pragma_link_name is
    procedure C_Function;
    pragma Link_Name ("c_func_name", C_Function);
begin
    null;
end pragma_link_name;