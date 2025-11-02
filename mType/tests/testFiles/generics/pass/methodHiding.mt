import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method hiding (not overriding)
class Base {
    public function <T> process(T value): String {
        return new String("Base: " + value);
    }
}

class Derived extends Base {
    // Hides base class method
    public function <T> process(T value): String {
        return new String("Derived: " + value);
    }
}

function main(): void {
    Derived d = new Derived();
    print(d.process(new Int(42)));
    print(d.process(new String("test")));
}

main();
