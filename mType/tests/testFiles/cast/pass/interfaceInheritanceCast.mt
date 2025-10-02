// Test: Interface inheritance casting
interface Base {
    void baseMethod();
}

interface Extended extends Base {
    void extendedMethod();
}

class Impl implements Extended {
    void baseMethod() { print("Base"); }
    void extendedMethod() { print("Extended"); }
}

Impl obj = new Impl();
Base base = (Base)obj;
base.baseMethod();

// Expected output:
// Base
