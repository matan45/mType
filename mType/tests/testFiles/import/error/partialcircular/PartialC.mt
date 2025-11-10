import { PartialA } from "./PartialA.mt"

class PartialC {
    fun useA(): String {
        var a = PartialA();
        return "C uses A (CIRCULAR!)";
    }
}
