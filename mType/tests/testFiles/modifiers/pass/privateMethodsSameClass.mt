// Test: Private methods accessible within same class
class Validator {
    private bool isValidLength(string text) {
        return text.length() >= 3;
    }

    private bool isValidFormat(string text) {
        return text != "";
    }

    public bool validate(string text) {
        // Calling private methods from same class
        return isValidLength(text) && isValidFormat(text);
    }
}

Validator v = new Validator();
print(v.validate("abc"));   // Expected: true
print(v.validate("ab"));    // Expected: false
print(v.validate("test"));  // Expected: true
