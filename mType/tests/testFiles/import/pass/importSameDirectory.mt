// Test: Import from same directory without ./
import { ExportedClass } from "modules/SameDirectoryModule.mt";

ExportedClass obj = new ExportedClass(42);
print(obj.getValue());
