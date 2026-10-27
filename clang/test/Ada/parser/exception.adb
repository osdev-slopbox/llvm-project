// RUN: %clang_ada -fsyntax-only %s

procedure Test_Exception is
    My_Error : exception;
begin
    raise My_Error with "something went wrong";
exception
    when My_Error =>
        null;
    when others =>
        null;
end Test_Exception;