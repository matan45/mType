import { PartialB } from "./PartialB.mt"

class PartialA {
    fun useB(): String {
        var b = PartialB();
        return "A uses B";
    }
}
