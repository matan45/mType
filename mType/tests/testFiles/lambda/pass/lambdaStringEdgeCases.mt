// Null string, empty string handling test
interface Function<T, R> {
    function apply(T input) : R;
}

print("=== String Edge Cases Test ===");

// Lambda handling null and empty strings
Function<String, String> safeToUpper = s -> {
    if (s == null) {
        return "NULL";
    }
    if (s.length() == 0) {
        return "EMPTY";
    }
    return s.toUpperCase();
};

print("Normal: " + safeToUpper.apply("hello"));
print("Null: " + safeToUpper.apply(null));
print("Empty: " + safeToUpper.apply(""));

// Lambda with string concatenation edge cases
Function<String, String> safeConcat = s -> {
    if (s == null) {
        return "Value is null";
    }
    return "Value is " + s;
};

print(safeConcat.apply("test"));
print(safeConcat.apply(null));
print(safeConcat.apply(""));

// Lambda with string length checks
Function<String, int> categorizeLength = s -> {
    if (s == null) {
        return -1;
    }
    if (s.length() == 0) {
        return 0;
    }
    if (s.length() < 5) {
        return 1;
    }
    if (s.length() < 10) {
        return 2;
    }
    return 3;
};

print("Category 'hi': " + categorizeLength.apply("hi"));
print("Category 'hello': " + categorizeLength.apply("hello"));
print("Category 'hello world': " + categorizeLength.apply("hello world"));
print("Category null: " + categorizeLength.apply(null));
print("Category '': " + categorizeLength.apply(""));

// Lambda with string comparison
Function<String, bool> isValidName = name -> {
    if (name == null) {
        return false;
    }
    if (name.length() == 0) {
        return false;
    }
    if (name.length() > 50) {
        return false;
    }
    return true;
};

print("Valid 'John': " + isValidName.apply("John"));
print("Valid null: " + isValidName.apply(null));
print("Valid '': " + isValidName.apply(""));
print("Valid 'A': " + isValidName.apply("A"));

// Lambda with whitespace-like handling
Function<String, String> normalize = s -> {
    if (s == null) {
        return "";
    }
    if (s.length() == 0) {
        return "";
    }
    return s;
};

String n1 = normalize.apply("test");
String n2 = normalize.apply(null);
String n3 = normalize.apply("");

print("Normalized 'test': '" + n1 + "'");
print("Normalized null: '" + n2 + "'");
print("Normalized '': '" + n3 + "'");

print("String edge cases complete");
