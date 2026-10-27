// RUN: %clang_ada -a2012 %ada_link_flags -o %t_prog %s 2> %t_build
// RUN: timeout 5 %t_prog 2>&1 | FileCheck %s
// Test: Controlled types with Initialize and Finalize

procedure Controlled_Test is
   type Controlled is tagged null record;

   procedure Initialize(Self : access Controlled) is
   begin
      put_line("initialized");
   end Initialize;

   procedure Finalize(Self : access Controlled) is
   begin
      put_line("finalized");
   end Finalize;

   X : Controlled;
begin
   -- Object created: Initialize should be called
   put_line("before exit");
   -- Object destroyed: Finalize should be called
end Controlled_Test;

// CHECK: initialized
// CHECK-NEXT: before exit
// CHECK-NEXT: finalized
