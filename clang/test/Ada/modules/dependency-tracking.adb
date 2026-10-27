// RUN: true
with Dep;

procedure Dep_Tracking is
begin
   Dep.Do_Something;
end Dep_Tracking;
