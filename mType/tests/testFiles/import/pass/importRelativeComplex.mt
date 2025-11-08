// Test: Complex relative paths with multiple directories

import { RelativeClass1 } from "modules/sub1/RelativeModule1.mt";
import { RelativeClass2 } from "modules/sub2/RelativeModule2.mt";

RelativeClass1 obj1 = new RelativeClass1();
RelativeClass2 obj2 = new RelativeClass2();
print(obj1.getValue() + obj2.getValue());
