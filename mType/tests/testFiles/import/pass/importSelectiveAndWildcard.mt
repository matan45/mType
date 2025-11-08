// Test: Both selective and wildcard from same file (if supported)
import { SelectiveClass1 } from "./modules/SelectiveModule.mt";
import * from "./modules/SelectiveModule.mt";

SelectiveClass1 obj1 = new SelectiveClass1();
SelectiveClass2 obj2 = new SelectiveClass2();
SelectiveClass3 obj3 = new SelectiveClass3();

print(obj1.test1());
print(obj2.test2());
print(obj3.test3());
