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
        return new String("NULL");
    }
    if (s.length() == 0) {
        return new String("EMPTY");
    }
    return new String(toUpperCase(s.getValue()));
};

print("Normal: " + safeToUpper.apply(new String("hello")));
print("Null: " + safeToUpper.apply(null));
print("Empty: " + safeToUpper.apply(new String("")));

// Lambda with string concatenation edge cases
Function<String, String> safeConcat = s -> {
    if (s == null) {
        return new String("Value is null");
    }
    return new String("Value is " + s);
};

print(safeConcat.apply(new String("test")));
print(safeConcat.apply(null));
print(safeConcat.apply(new String("")));

// Lambda with string length checks
Function<String, Int> categorizeLength = s -> {
    if (s == null) {
        return new Int(-1);
    }
    if (s.length() == 0) {
        return new Int(0);
    }
    if (s.length() < 5) {
        return new Int(1);
    }
    if (s.length() < 10) {
        return new Int(2);
    }
    return new Int(3);
};

print("Category 'hi': " + categorizeLength.apply(new String("hi")).getValue());
print("Category 'hello': " + categorizeLength.apply(new String("hello")).getValue());
print("Category 'hello world': " + categorizeLength.apply(new String("hello world")).getValue());
print("Category null: " + categorizeLength.apply(null).getValue());
print("Category '': " + categorizeLength.apply(new String("")).getValue());

// Lambda with string comparison
Function<String, Bool> isValidName = name -> {
    if (name == null) {
        return new Bool(false);
    }
    if (name.length() == 0) {
        return new Bool(false);
    }
    if (name.length() > 50) {
        return new Bool(false);
    }
    return new Bool(true);
};

print("Valid 'John': " + isValidName.apply(new String("John")).getValue());
print("Valid null: " + isValidName.apply(null).getValue());
print("Valid '': " + isValidName.apply(new String("")).getValue());
print("Valid 'A': " + isValidName.apply(new String("A")).getValue());

// Lambda with whitespace-like handling
Function<String, String> normalize = s -> {
    if (s == null) {
        return new String("");
    }
    if (s.length() == 0) {
        return new String("");
    }
    return s;
};

string n1 = normalize.apply(new String("test")).getValue();
string n2 = normalize.apply(null).getValue();
string n3 = normalize.apply(new String("")).getValue();

print("Normalized 'test': '" + n1 + "'");
print("Normalized null: '" + n2 + "'");
print("Normalized '': '" + n3 + "'");

print("String edge cases complete");
