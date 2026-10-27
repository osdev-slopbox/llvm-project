// REQUIRES: ada_runtime_link
// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: sh -c '%t_prog 2> %t_err; test $? = 134'
// RUN: FileCheck %s --check-prefix=ERR --input-file %t_err

// Tagged dispatch: each overload is called for its correct type.
// Describe(C) -> Circle_Called (caught)
// Describe(Q) -> Square_Called (caught)
// Describe(T) -> Constraint_Error (caught; no Triangle overload)
// Tag preserved through assignment: C2 := C; Describe(C2) -> Circle_Called
// Illegal assignment Circle := Square -> Storage_Error (caught)
// All pass -> raise All_Passed (unhandled, exit 134)

procedure Main is
   type Shape is tagged null record;
   type Circle is new Shape;
   type Square is new Shape;
   type Triangle is new Shape;

   procedure Describe(S : Shape) is
   begin
      raise Constraint_Error;
   end Describe;

   procedure Describe(S : Circle) is
   begin
      raise Circle_Called;
   end Describe;

   procedure Describe(S : Square) is
   begin
      raise Square_Called;
   end Describe;

   C : Circle;
   Q : Square;
   T : Triangle;
   C2 : Circle;
begin
   try
      Describe(C);
   exception
      when Circle_Called is
         null;
   end try;

   try
      Describe(Q);
   exception
      when Square_Called is
         null;
   end try;

   try
      Describe(T);
   exception
      when Constraint_Error is
         null;
   end try;

   C2 := C;
   try
      Describe(C2);
   exception
      when Circle_Called is
         null;
   end try;

   try
      C := Q;
   exception
      when Storage_Error is
         null;
   end try;

   try
      Describe(C);
   exception
      when Circle_Called is
         null;
   end try;

   raise All_Passed;
end Main;

ERR: ada: unhandled exception: All_Passed
