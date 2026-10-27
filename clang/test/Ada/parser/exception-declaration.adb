-- Test: exception declaration
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure exception_declaration is
    My_Exception : exception;
begin
    raise My_Exception;
end exception_declaration;
