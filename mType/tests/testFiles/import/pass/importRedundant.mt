// Test: Importing same symbol multiple times (should be allowed or warned)
@Script
import { RedundantClass } from "./modules/RedundantModule.mt"
import { RedundantClass } from "./modules/RedundantModule.mt"
import { RedundantClass } from "./modules/RedundantModule.mt"

var obj = RedundantClass();
print(obj.test());
