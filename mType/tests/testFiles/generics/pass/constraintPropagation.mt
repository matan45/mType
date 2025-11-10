import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Constraint propagation in inheritance hierarchy
interface Validator {
    function validate(): Bool;
}

class BaseContainer<T extends Validator> {
    T item;

    public function setItem(T i): void {
        item = i;
    }

    public function isValid(): Bool {
        return item.validate();
    }
}

class ExtendedContainer<T extends Validator> extends BaseContainer<T> {
    public function printValidation(): void {
        if (this.isValid().getValue()) {
            print("Valid");
        } else {
            print("Invalid");
        }
    }
}

class ValidatedItem implements Validator {
    Bool valid;

    public constructor(Bool v) {
        valid = v;
    }

    public function validate(): Bool {
        return valid;
    }
}

function main(): void {
    ExtendedContainer<ValidatedItem> container = new ExtendedContainer<ValidatedItem>();
    container.setItem(new ValidatedItem(new Bool(true)));
    container.printValidation();
}

main();
