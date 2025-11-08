// Test: Import from module that imports from another module
import { ModuleBClass } from "./modules/ReexportModuleB.mt";

ModuleBClass obj = new ModuleBClass();
print(obj.useReexported());
