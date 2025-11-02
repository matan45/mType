// Test: Import with explicit ./
@Script
import { DotSlashClass } from "./modules/DotSlashModule.mt"

var obj = DotSlashClass();
print(obj.greet());
