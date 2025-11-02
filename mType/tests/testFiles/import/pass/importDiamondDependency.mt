// Test: Diamond dependency - A imports B and C, both import D (should pass)
@Script
import { ClassB } from "./diamond/ModuleB.mt"
import { ClassC } from "./diamond/ModuleC.mt"

var b = ClassB();
var c = ClassC();
print(b.useShared());
print(c.useShared());
