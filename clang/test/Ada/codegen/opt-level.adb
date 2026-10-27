-- Test: optimization level flag (P11)
-- RUN: %clang_ada -a2012 -O2 -c %s -o %t.o

procedure test_opt is
    X : Integer := 42;
begin
    null;
end test_opt;