// Test: Diamond inheritance across files
// @Script

import "modules/InterfaceA.mt";
import "modules/InterfaceB.mt";
import "modules/InterfaceC.mt";

class DiamondImpl : InterfaceB, InterfaceC {
    methodA() : String {
        return "Method A";
    }

    methodB() : String {
        return "Method B";
    }

    methodC() : String {
        return "Method C";
    }
}

testAsA(obj: InterfaceA) : String {
    return obj.methodA();
}

testAsB(obj: InterfaceB) : String {
    return obj.methodA() + " + " + obj.methodB();
}

testAsC(obj: InterfaceC) : String {
    return obj.methodA() + " + " + obj.methodC();
}

main() : Void {
    let impl = new DiamondImpl();

    let resultA = testAsA(impl);
    print("As A: " + resultA);
    assert(resultA == "Method A", "Should work as InterfaceA");

    let resultB = testAsB(impl);
    print("As B: " + resultB);
    assert(resultB == "Method A + Method B", "Should work as InterfaceB");

    let resultC = testAsC(impl);
    print("As C: " + resultC);
    assert(resultC == "Method A + Method C", "Should work as InterfaceC");

    print("Diamond inheritance test passed");
}
