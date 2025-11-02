import { ClassB } from "./ModuleB.mt"

class ClassA {
    fun useB(): String {
        var b = ClassB();
        return "A uses " + b.useC();
    }
}
