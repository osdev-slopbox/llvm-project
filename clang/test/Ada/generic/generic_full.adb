// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic subprogram with spec, body, and instantiation (RM 12.2, 12.3, 12.5)
// Tests the full generic pipeline.

generic
   type Element is private;
   with function ">"(Left, Right : in Element) return Boolean;
function Max(A, B : in Element) return Element;

-- Generic body
generic
   type Element is private;
   with function ">"(Left, Right : in Element) return Boolean;
function Max(A, B : in Element) return Element is
begin
   if A > B then
      return A;
   else
      return B;
   end if;
end Max;

-- Instantiation with Integer
function Max_Int is new Max(
   Element => Integer,
   ">" => "gt"
);
