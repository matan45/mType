import { SharedClass } from "./ModuleD.mt"

class ClassB {
    fun useShared(): String {
        var shared = SharedClass();
        return "B: " + shared.getValue();
    }
}
