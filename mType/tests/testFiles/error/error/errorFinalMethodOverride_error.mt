// Overriding a `final` method in a subclass must be a compile error.
import * from "../../lib/exceptions/Exception.mt";

class Base {
    public final function run(): void {
        print("base");
    }
}

class Sub extends Base {
    public function run(): void {
        throw new Exception("sub override of final");
    }
}

function main(): void {
    Base b = new Sub();
    b.run();
}
main();
