import { IndirectB } from "./IndirectB.mt"

class IndirectA {
    fun useB(): String {
        var b = IndirectB();
        return "A -> " + b.useC();
    }
}
