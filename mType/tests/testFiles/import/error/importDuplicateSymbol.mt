// Test: Same symbol imported twice from same file
@Script
import { DuplicateClass } from "./modules/DuplicateModule.mt"
import { DuplicateClass } from "./modules/DuplicateModule.mt"

var obj = DuplicateClass();
print(obj.test());
