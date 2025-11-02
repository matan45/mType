// Test: Indirect circular - A->B->C->A
@Script
import { IndirectA } from "./indirectcircular/IndirectA.mt"

var a = IndirectA();
print(a.useB());
