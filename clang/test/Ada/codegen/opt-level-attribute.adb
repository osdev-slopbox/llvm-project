-- Test: optimization level attribute in IR (P11)
-- RUN: %clang_ada -a2012 -O2 -c %s -o %t.o
-- RUN: %clang_ada -a2012 -O0 -c %s -o %t2.o

procedure test_opt_attr is
    X : Integer := 42;
begin
    null;
end test_opt_attr;