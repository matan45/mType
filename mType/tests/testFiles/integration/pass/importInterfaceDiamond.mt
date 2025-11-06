// Test: Diamond inheritance across files
// @Script

import "modules/InterfaceA.mt";
import "modules/InterfaceB.mt";
import "modules/InterfaceC.mt";

class DiamondImpl implements InterfaceB, InterfaceC {
    public function methodA() : String {
        return "Method A";
    }

    public function methodB() : String {
        return "Method B";
    }

    public function methodC() : String {
        return "Method C";
    }
}

function testAsA(obj: InterfaceA) : String {
    return obj.methodA();
}

function testAsB(obj: InterfaceB) : String {
    return obj.methodA() + " + " + obj.methodB();
}

function testAsC(obj: InterfaceC) : String {
    return obj.methodA() + " + " + obj.methodC();
}

function main() : void {
    DiamondImpl impl = new DiamondImpl();

    String resultA = testAsA(impl);
    print("As A: " + resultA);
    assert(resultA == "Method A", "Should work as InterfaceA");

    String resultB = testAsB(impl);
    print("As B: " + resultB);
    assert(resultB == "Method A + Method B", "Should work as InterfaceB");

    String resultC = testAsC(impl);
    print("As C: " + resultC);
    assert(resultC == "Method A + Method C", "Should work as InterfaceC");

    print("Diamond inheritance test passed");
}
