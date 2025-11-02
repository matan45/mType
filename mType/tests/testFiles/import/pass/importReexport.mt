// Test: Import from module that imports from another module
@Script
import { ModuleBClass } from "./modules/ReexportModuleB.mt"

var obj = ModuleBClass();
print(obj.useReexported());
