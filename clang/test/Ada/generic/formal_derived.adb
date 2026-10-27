// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic with formal derived type (RM 12.2.1)
// The formal type is derived from a specific type.

generic
   type T is new Integer;
procedure Foo(X : T);

-- Instantiation with a derived type
type My_Int is new Integer;
procedure Foo_My_Int is new Foo(T => My_Int);
