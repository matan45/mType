// Test: Import from a file that has syntax errors
import { BrokenClass } from "./modules/BrokenSyntaxModule.mt"

var obj = BrokenClass();
print(obj.test());
