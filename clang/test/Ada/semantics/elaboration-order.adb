-- Test: elaboration order analysis (P10)
-- RUN: %clang_ada -a2012 -fsyntax-only %s

with Elaborated_Package;
pragma Elaborate (Elaborated_Package);

procedure test_elaboration_order is
begin
    null;
end test_elaboration_order;