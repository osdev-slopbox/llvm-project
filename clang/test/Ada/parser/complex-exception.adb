-- Test: complex exception handling
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_exception is
    My_Exception : exception;
    Another_Exception : exception;
begin
    try
        try
            raise My_Exception;
        exception
            when My_Exception =>
                raise Another_Exception;
        end;
    exception
        when Another_Exception =>
            null;
    end;
end complex_exception;
