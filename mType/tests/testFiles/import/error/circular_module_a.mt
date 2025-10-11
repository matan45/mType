// Circular import test - Module A
// This file imports from circular_module_b.mt
// which in turn imports from this file
// This should cause a circular dependency error

import { ClassB, functionB } from "circular_module_b.mt";

public class ClassA {
    function useB() : int {
        ClassB b = new ClassB();
        return b.getValue();
    }
}

public function functionA() : int {
    return functionB() + 1;
}
