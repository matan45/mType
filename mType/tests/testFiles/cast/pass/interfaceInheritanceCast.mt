// Test: Interface inheritance casting
interface Base {
    public function baseMethod(): void;
}

interface Extended extends Base {
    public function extendedMethod(): void;
}

class Impl implements Extended {
    public function baseMethod(): void { print("Base"); }
    public function extendedMethod(): void { print("Extended"); }
}

Impl obj = new Impl();
Base base = (Base)obj;
base.baseMethod();

// Expected output:
// Base
