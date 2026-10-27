// RUN: %clang_ada -fsyntax-only -a2012 %s
// Generic subprogram specification (RM 12.2)

generic
   type Element is private;
procedure Print_All(Items : in array of Element);

// Generic body is a separate library item (in a .adb file)
// For testing, we just parse the spec here.
