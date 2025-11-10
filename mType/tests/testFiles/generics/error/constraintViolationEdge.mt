import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Edge case constraint violation
interface Validator {
    function validate(): Bool;
}

class Container<T extends Validator> {
    T item;

    public function setItem(T i): void {
        item = i;
    }
}

class NonValidator {
    String data;

    public function NonValidator(String d) {
        data = d;
    }
}

function main(): void {
    // Error: NonValidator does not implement Validator
    Container<NonValidator> container = new Container<NonValidator>();
    container.setItem(new NonValidator(new String("test")));
}

main();
