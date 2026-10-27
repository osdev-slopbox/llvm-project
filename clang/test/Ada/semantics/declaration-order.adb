-- Test: declaration order
-- RUN: %clang_ada -a2012 -fsyntax-only %s

procedure declaration_order is
    x : Integer := 5;
    y : Integer := x + 1;
begin
    null;
end declaration_order;
