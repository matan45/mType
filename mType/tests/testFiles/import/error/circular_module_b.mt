// Circular import test - Module B
// This file imports from circular_module_a.mt
// which imports from this file
// This creates a circular dependency

import { ClassA, functionA } from "circular_module_a.mt"

public class ClassB {
    function getValue() : int {
        return 42;
    }

    function useA() : int {
        ClassA a = new ClassA();
        return a.useB();
    }
}

public function functionB() : int {
    return functionA() - 1;
}
