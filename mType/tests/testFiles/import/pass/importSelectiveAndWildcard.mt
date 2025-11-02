// Test: Both selective and wildcard from same file (if supported)
@Script
import { SelectiveClass1 } from "./modules/SelectiveModule.mt"
import * from "./modules/SelectiveModule.mt"

var obj1 = SelectiveClass1();
var obj2 = SelectiveClass2();
var obj3 = SelectiveClass3();

print(obj1.test1());
print(obj2.test2());
print(obj3.test3());
