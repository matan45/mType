// Test: @Throw annotation on class methods
// Expected: Should compile and validate successfully
import * from "../../lib/exceptions/Exception.mt";
class ValidationException extends Exception {
    constructor(string message): super(message) {
    }
}

class UserService {
    @Throw(ValidationException)
    public function validateUser(string username): bool {
        // Method declares it may throw ValidationException
        if (username == "") {
            return false;
        }
        return true;
    }
}

// Test execution
UserService service = new UserService();
bool isValid = service.validateUser("john");
print("Validation result: " + isValid);
