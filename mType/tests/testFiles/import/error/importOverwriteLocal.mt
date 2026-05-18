// Test: Import conflicts with local definition
import { LocalClass } from "./modules/OverwriteModule.mt"

class LocalClass {
    fun getSource(): String {
        return "Local definition";
    }
}

var obj = LocalClass();
print(obj.getSource());
