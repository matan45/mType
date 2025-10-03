// Test: Interface inheritance casting
interface Base {
    function baseMethod(): void;
}

interface Extended extends Base {
    function extendedMethod(): void;
}

class Impl implements Extended {
    function baseMethod(): void { print("Base"); }
    function extendedMethod(): void { print("Extended"); }
}

Impl obj = new Impl();
Base base = (Base)obj;
base.baseMethod();

// Expected output:
// Base
