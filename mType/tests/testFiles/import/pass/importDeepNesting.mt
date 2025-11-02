// Test: Multiple ../ levels
@Script
import { DeepClass } from "./deep/nested/path/DeepModule.mt"

var obj = DeepClass();
print(obj.getDepth());
