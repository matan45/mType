// Test: Private methods accessible within same class
class Validator {
    private function isValidLength(string text): bool {
        return strLength(text) >= 3;
    }

    private function isValidFormat(string text): bool {
        return text != "";
    }

    public function validate(string text): bool {
        // Calling private methods from same class
        return isValidLength(text) && isValidFormat(text);
    }
}

Validator v = new Validator();
print(v.validate("abc"));   // Expected: true
print(v.validate("ab"));    // Expected: false
print(v.validate("test"));  // Expected: true
