import "../../lib/primitives/String.mt";

// Non-generic function
function regularFunc(String value): String {
    return value;
}

// Error: Calling non-generic function with type arguments
String result = regularFunc<String>(new String("test"));
