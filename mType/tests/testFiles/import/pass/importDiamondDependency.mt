// Test: Diamond dependency - A imports B and C, both import D (should pass)
import { ClassB } from "./diamond/ModuleB.mt";
import { ClassC } from "./diamond/ModuleC.mt";

ClassB b =new ClassB();
ClassC c = new ClassC();
print(b.useShared());
print(c.useShared());
