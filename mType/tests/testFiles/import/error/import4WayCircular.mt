// Test: 4-module circular dependency chain (A->B->C->D->A)
@Script
import { ClassA } from "./circular4way/ModuleA.mt"

var a = ClassA();
print(a.useB());
