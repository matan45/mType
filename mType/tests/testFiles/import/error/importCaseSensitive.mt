// Test: Case sensitivity - should fail on case-sensitive file systems
import { CaseSensitiveClass } from "./modules/CaseSensitive.mt"

var obj = CaseSensitiveClass();
print(obj.test());
