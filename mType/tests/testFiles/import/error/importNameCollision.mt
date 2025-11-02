// Test: Same class name from different files
@Script
import { Collision } from "./modules/CollisionModule1.mt"
import { Collision } from "./modules/CollisionModule2.mt"

var obj = Collision();
print(obj.getSource());
