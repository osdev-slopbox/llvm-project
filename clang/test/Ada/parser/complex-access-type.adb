-- Test: complex access type
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure complex_access_type is
    type Node;
    type Node_Ptr is access Node;
    type Node is record
        value : Integer;
        left : Node_Ptr;
        right : Node_Ptr;
    end record;

    function make_node(value : Integer) return Node_Ptr is
        node : Node_Ptr := new Node;
    begin
        node.value := value;
        node.left := null;
        node.right := null;
        return node;
    end make_node;
begin
    null;
end complex_access_type;
