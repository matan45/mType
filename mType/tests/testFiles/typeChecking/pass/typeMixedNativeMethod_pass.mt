// Test native method type checking
class StringUtils {
    // Native method declarations should be type-checked
    native string toUpperCase(string s);
    native int length(string s);
    native bool isEmpty(string s);

    // Regular methods using native methods
    string processString(string input): string {
        if (isEmpty(input)) {
            return "empty";
        }
        int len = length(input);
        if (len > 5) {
            return toUpperCase(input);
        }
        return input;
    }
}

StringUtils utils = new StringUtils();
string result = utils.processString("hello world");
print("Native method type checking passed");
