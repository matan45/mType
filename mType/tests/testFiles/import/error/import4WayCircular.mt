// Test: 4-module circular dependency chain (A->B->C->D->A)
import { ClassA } from "./circular4way/ModuleA.mt"

var a = ClassA();
print(a.useB());
