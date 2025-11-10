// Test: Trailing comma in import list (may pass or error depending on parser)
@Script
import { TrailingCommaClass, } from "./modules/TrailingCommaModule.mt"

var obj = TrailingCommaClass();
print(obj.test());
