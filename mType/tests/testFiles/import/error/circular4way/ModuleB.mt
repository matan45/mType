import { ClassC } from "./ModuleC.mt"

class ClassB {
    fun useC(): String {
        var c = ClassC();
        return "B uses " + c.useD();
    }
}
