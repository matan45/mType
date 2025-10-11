// Three-way circular dependency test - Module A
// A imports B, B imports C, C imports A
// This should detect the circular dependency

import { ClassB } from "circular_three_way_b.mt"

public class ClassA {
    function createB() : object {
        ClassB b = new ClassB();
        return b;
    }
}

public function helperA() : int {
    return 1;
}
