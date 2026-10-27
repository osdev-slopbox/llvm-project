// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic subprogram specification (RM 12.2)

generic
   type T is private;
function Max(A, B : T) return T;

-- Instantiation (RM 12.5)
function Max_Int is new Max(T => Integer);
