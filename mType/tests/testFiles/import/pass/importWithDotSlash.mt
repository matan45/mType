// Test: Import with explicit ./
import { DotSlashClass } from "./modules/DotSlashModule.mt";

DotSlashClass obj = new DotSlashClass();
print(obj.greet());
