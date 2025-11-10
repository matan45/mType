import { IndirectC } from "./IndirectC.mt"

class IndirectB {
    fun useC(): String {
        var c = IndirectC();
        return "B -> " + c.useA();
    }
}
