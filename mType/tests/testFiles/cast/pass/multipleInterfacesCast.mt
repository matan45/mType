// Test: Cast with multiple interfaces
interface A {
    function methodA(): void;
}

interface B {
    function methodB():void;
}

class Multi implements A, B {
    public function methodA(): void { print("A"); }
    public function methodB(): void { print("B"); }
}

Multi obj = new Multi();
A interfaceA = (A)obj;
interfaceA.methodA();
B interfaceB = (B)obj;
interfaceB.methodB();

// Expected output:
// A
// B
