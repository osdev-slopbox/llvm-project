// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic subprogram with instantiation and call (RM 12.2, 12.3, 12.5)
// Tests calling an instantiated generic function.

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

procedure Main is
   R : Integer;
begin
   R := Max_Int(5, 10);
end Main;
