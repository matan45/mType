import { IndirectA } from "./IndirectA.mt"

class IndirectC {
    fun useA(): String {
        var a = IndirectA();
        return "C -> A (CIRCULAR!)";
    }
}
