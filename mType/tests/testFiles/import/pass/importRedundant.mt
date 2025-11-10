// Test: Importing same symbol multiple times (should be allowed or warned)

import { RedundantClass } from "./modules/RedundantModule.mt";
import { RedundantClass } from "./modules/RedundantModule.mt";
import { RedundantClass } from "./modules/RedundantModule.mt";

RedundantClass obj = new RedundantClass();
print(obj.test());
