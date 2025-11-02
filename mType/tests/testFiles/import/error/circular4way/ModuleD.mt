import { ClassA } from "./ModuleA.mt"

class ClassD {
    fun useA(): String {
        var a = ClassA();
        return "D uses A (CIRCULAR!)";
    }
}
