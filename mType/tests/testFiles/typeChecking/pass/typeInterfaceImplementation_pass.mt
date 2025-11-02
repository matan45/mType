// Test interface implementation validation with proper type checking
interface Validator<T> {
    function validate(T value): bool;
    function getMessage(): string;
}

class StringValidator implements Validator<string> {
    string message;

    constructor() {
        this.message = "Invalid string";
    }

    public function validate(string value): bool {
        return value != null && value.length() > 0;
    }

    public function getMessage(): string {
        return this.message;
    }
}

class IntValidator implements Validator<int> {
    int minValue;

    constructor(int min) {
        this.minValue = min;
    }

    public function validate(int value): bool {
        return value >= this.minValue;
    }

    public function getMessage(): string {
        return "Value must be >= " + this.minValue;
    }
}

StringValidator strValidator = new StringValidator();
IntValidator intValidator = new IntValidator(0);

print(strValidator.validate("test"));
print(strValidator.getMessage());
print(intValidator.validate(5));
print(intValidator.getMessage());
print("Interface implementation validation successful");
