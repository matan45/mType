// Test nullable interface type (if supported)
// @Script

interface Validator {
    func validate(input: String): Bool;
}

class EmailValidator implements Validator {
    func validate(input: String): Bool {
        // Simple validation - just check for @ symbol
        return input.contains("@");
    }
}

class ValidationService {
    var validator: Validator;

    func init() {
        this.validator = null;  // Explicitly nullable
    }

    func setValidator(v: Validator): void {
        this.validator = v;
    }

    func isValid(input: String): Bool {
        if (this.validator == null) {
            print("No validator set, assuming valid");
            return true;
        }
        return this.validator.validate(input);
    }
}

var service = new ValidationService();

// Test with null validator
print(service.isValid("test"));  // Should print message and return true

// Set validator and test
service.setValidator(new EmailValidator());
print(service.isValid("test@example.com"));  // Should return true
print(service.isValid("invalid"));           // Should return false
