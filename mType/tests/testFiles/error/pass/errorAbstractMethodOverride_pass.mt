// Test: try/catch around an abstract method override that throws.
// The base-class reference dispatches to the subclass implementation,
// which throws; the exception is caught at the call site.
import * from "../../lib/exceptions/Exception.mt";

abstract class Validator {
    abstract function validate(int v): void;
}

class PositiveValidator extends Validator {
    public function validate(int v): void {
        if (v < 0) {
            throw new Exception("Negative: " + v);
        }
    }
}

function main(): void {
    Validator val = new PositiveValidator();
    try {
        val.validate(-5);
    } catch (Exception e) {
        print("caught " + e.getMessage());
    }
    try {
        val.validate(7);
        print("ok 7");
    } catch (Exception e) {
        print("unexpected");
    }
}
main();
