import * from "./MixedB.mt"

class MixedA {
    fun test(): String {
        var b = MixedB();
        return "A uses B";
    }
}
