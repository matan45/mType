// Test: Import from parent directory using ../

import { ParentClass } from "../ParentModule.mt";

ParentClass obj = new ParentClass("Parent");
print(obj.getName());
