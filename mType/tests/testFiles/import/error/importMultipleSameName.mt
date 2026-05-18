// Test: Importing same class name from 2 different files
import { SameName } from "./modules/SameName1.mt"
import { SameName } from "./modules/SameName2.mt"

var obj = SameName();
print(obj.getSource());
