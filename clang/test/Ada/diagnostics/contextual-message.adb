-- Test: contextual parser error messages (P8)
-- RUN: %clang_ada -a2012 -fsyntax-only %s 2>&1 | FileCheck %s
-- CHECK: expected ';' but found 'end'

procedure Contextual_Message is
   X : Integer := 0;
begin
   X := 1
end Contextual_Message;
