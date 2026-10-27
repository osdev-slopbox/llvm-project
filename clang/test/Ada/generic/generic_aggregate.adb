// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic with aggregate formal parameter (RM 12.2.2)
// The formal parameter is an array type.

generic
   type Element is private;
   with function "+"(Left, Right : in Element) return Element;
procedure Sum(Items : in array of Element);
