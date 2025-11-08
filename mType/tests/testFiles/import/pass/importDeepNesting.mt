// Test: Multiple ../ levels
import { DeepClass } from "deep/nested/path/DeepModule.mt";

DeepClass obj = new DeepClass();
print(obj.getDepth());
