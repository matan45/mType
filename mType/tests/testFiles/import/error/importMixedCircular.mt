// Test: Mixed selective and wildcard circular import
import { MixedA } from "./mixedcircular/MixedA.mt"

var a = MixedA();
print(a.test());
