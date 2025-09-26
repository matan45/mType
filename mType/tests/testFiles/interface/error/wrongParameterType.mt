// Wrong parameter type in interface implementation
interface Validator {
    function validate(string input): bool;
}

class BadValidator implements Validator {
    function validate(int input): bool {  // Wrong parameter type - should be string
        return true;
    }
}

print("This should not print");