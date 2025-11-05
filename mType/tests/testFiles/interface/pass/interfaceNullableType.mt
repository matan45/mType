// Test nullable interface type (if supported)
// @Script

interface Validator {
    function validate(string input): bool;
}

class EmailValidator implements Validator {
    public function validate(string input): bool {
        // Simple validation - check if string length is reasonable for an email
        // (Proper @ check would require string indexing which may not be available)
        return strLength(input) > 7;
    }
}

class ValidationService {
    private Validator validator;

    public constructor() {
        this.validator = null;  // Explicitly nullable
    }

    public function setValidator(Validator v): void {
        this.validator = v;
    }

    public function isValid(string input): bool {
        if (this.validator == null) {
            print("No validator set, assuming valid");
            return true;
        }
        return this.validator.validate(input);
    }
}

ValidationService service = new ValidationService();

// Test with null validator
print(service.isValid("test"));  // Should print message and return true

// Set validator and test
service.setValidator(new EmailValidator());
print(service.isValid("test@example.com"));  // Should return true
print(service.isValid("invalid"));           // Should return false
