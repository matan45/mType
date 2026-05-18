// Test: Indirect circular - A->B->C->A
import { IndirectA } from "./indirectcircular/IndirectA.mt"

var a = IndirectA();
print(a.useB());
