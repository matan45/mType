import { SharedClass } from "./ModuleD.mt"

class ClassC {
    fun useShared(): String {
        var shared = SharedClass();
        return "C: " + shared.getValue();
    }
}
