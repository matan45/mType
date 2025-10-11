// Three-way circular dependency test - Module C
// A imports B, B imports C, C imports A
// This completes the circular dependency chain

import { ClassA } from "circular_three_way_a.mt";

public class ClassC {
    function createA() : object {
        ClassA a = new ClassA();
        return a;
    }
}

public function helperC() : int {
    return 3;
}
