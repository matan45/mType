import { ClassD } from "./ModuleD.mt"

class ClassC {
    fun useD(): String {
        var d = ClassD();
        return "C uses " + d.useA();
    }
}
