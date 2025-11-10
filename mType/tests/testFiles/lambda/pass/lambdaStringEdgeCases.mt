// Null string, empty string handling test
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

print("=== String Edge Cases Test ===");

// Lambda handling null and empty strings
Function<String, String> safeToUpper = s -> {
    if (s == null) {
        return "NULL";
    }
    if (strLength(s) == 0) {
        return "EMPTY";
    }
    return toUpperCase(s);
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
Function<String, Int> categorizeLength = s -> {
    if (s == null) {
        return new Int(-1);
    }
    if (strLength(s) == 0) {
        return new Int(0);
    }
    if (strLength(s) < 5) {
        return new Int(1);
    }
    if (strLength(s) < 10) {
        return new Int(2);
    }
    return new Int(3);
};

print("Category 'hi': " + categorizeLength.apply("hi").getValue());
print("Category 'hello': " + categorizeLength.apply("hello").getValue());
print("Category 'hello world': " + categorizeLength.apply("hello world").getValue());
print("Category null: " + categorizeLength.apply(null).getValue());
print("Category '': " + categorizeLength.apply("").getValue());

// Lambda with string comparison
Function<String, Bool> isValidName = name -> {
    if (name == null) {
        return new Bool(false);
    }
    if (strLength(name) == 0) {
        return new Bool(false);
    }
    if (strLength(name) > 50) {
        return new Bool(false);
    }
    return new Bool(true);
};

print("Valid 'John': " + isValidName.apply("John").getValue());
print("Valid null: " + isValidName.apply(null).getValue());
print("Valid '': " + isValidName.apply("").getValue());
print("Valid 'A': " + isValidName.apply("A").getValue());

// Lambda with whitespace-like handling
Function<String, String> normalize = s -> {
    if (s == null) {
        return "";
    }
    if (strLength(s) == 0) {
        return "";
    }
    return s;
};

string n1 = normalize.apply("test");
string n2 = normalize.apply(null);
string n3 = normalize.apply("");

print("Normalized 'test': '" + n1 + "'");
print("Normalized null: '" + n2 + "'");
print("Normalized '': '" + n3 + "'");

print("String edge cases complete");
