// Test: Import from same directory without ./
@Script
import { ExportedClass } from "modules/SameDirectoryModule.mt"

var obj = ExportedClass(42);
print(obj.getValue());
