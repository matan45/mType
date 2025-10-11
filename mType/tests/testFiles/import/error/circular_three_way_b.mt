// Three-way circular dependency test - Module B
// A imports B, B imports C, C imports A
// This should detect the circular dependency

import { ClassC } from "circular_three_way_c.mt";

public class ClassB {
    function createC() : object {
        ClassC c = new ClassC();
        return c;
    }
}

public function helperB() : int {
    return 2;
}
