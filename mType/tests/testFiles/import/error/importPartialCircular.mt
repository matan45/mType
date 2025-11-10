// Test: Partial circular - A imports B, B imports C, C imports A
@Script
import { PartialA } from "./partialcircular/PartialA.mt"

var a = PartialA();
print(a.useB());
