// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic subprogram with contracts (RM 12.2, 5.5.1)
// Tests that Pre/Post aspects work with generic subprograms.

generic
   type Element is private;
   with function ">"(Left, Right : in Element) return Boolean;
   with function "="(Left, Right : in Element) return Boolean;
function Max(A, B : in Element) return Element
   with Pre => True,
        Post => "Result" = A or else "Result" = B;
