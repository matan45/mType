// Test interface implementation validation with proper type checking
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Validator<T> {
    public function validate(T value): bool;
    public function getMessage(): string;
}

class StringValidator implements Validator<String> {
    string message;

    constructor() {
        this.message = "Invalid string";
    }

    public function validate(String value): bool {
        return value != null && value.length() > 0;
    }

    public function getMessage(): string {
        return this.message;
    }
}

class IntValidator implements Validator<Int> {
    int minValue;

    constructor(int min) {
        this.minValue = min;
    }

    public function validate(Int value): bool {
        return value.getValue() >= this.minValue;
    }

    public function getMessage(): string {
        return "Value must be >= " + this.minValue;
    }
}

StringValidator strValidator = new StringValidator();
IntValidator intValidator = new IntValidator(0);

print(strValidator.validate("test"));
print(strValidator.getMessage());
print(intValidator.validate(new Int(5)));
print(intValidator.getMessage());
print("Interface implementation validation successful");
