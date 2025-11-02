// Module B imports from A
import { ReexportedClass } from "./ReexportModuleA.mt"

// Module B can use it
class ModuleBClass {
    fun useReexported(): String {
        var obj = ReexportedClass();
        return obj.getValue();
    }
}
