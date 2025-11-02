// Test: Complex relative paths with multiple directories
@Script
import { RelativeClass1 } from "./modules/sub1/RelativeModule1.mt"
import { RelativeClass2 } from "./modules/sub2/RelativeModule2.mt"

var obj1 = RelativeClass1();
var obj2 = RelativeClass2();
print(obj1.getValue() + obj2.getValue());
