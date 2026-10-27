-- Test: complex access types
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_access is
    type Node;
    type Node_Ptr is access Node;
    type Node is record
        value : Integer;
        next : Node_Ptr;
    end record;
begin
    null;
end complex_access;
