// Test: Import from parent directory using ../
@Script
import { ParentClass } from "../ParentModule.mt"

var obj = ParentClass("Parent");
print(obj.getName());
