// Test: File importing itself (self-referential circular import)
import { SomeClass } from "./importCircularSelf.mt"

class SomeClass {
    fun test(): String {
        return "This should fail";
    }
}
