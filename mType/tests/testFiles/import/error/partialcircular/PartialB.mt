import { PartialC } from "./PartialC.mt"

class PartialB {
    fun useC(): String {
        var c = PartialC();
        return "B uses C";
    }
}
