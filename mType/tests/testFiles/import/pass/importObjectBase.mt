// Test: Import Object from lib
@Script
import { Object } from "../../../../../../lib/Object.mt"

class MyClass extends Object {
    fun test(): String {
        return "Inherits from Object";
    }
}

var obj = MyClass();
print(obj.test());
