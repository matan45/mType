import { MixedA } from "./MixedA.mt"

class MixedB {
    fun test(): String {
        var a = MixedA();
        return "B uses A (CIRCULAR!)";
    }
}
