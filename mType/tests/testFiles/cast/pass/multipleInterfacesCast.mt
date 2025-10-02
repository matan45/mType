// Test: Cast with multiple interfaces
interface A {
    void methodA();
}

interface B {
    void methodB();
}

class Multi implements A, B {
    void methodA() { print("A"); }
    void methodB() { print("B"); }
}

Multi obj = new Multi();
A interfaceA = (A)obj;
interfaceA.methodA();
B interfaceB = (B)obj;
interfaceB.methodB();

// Expected output:
// A
// B
