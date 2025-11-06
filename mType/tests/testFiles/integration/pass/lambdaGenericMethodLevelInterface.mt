// Test: Interface with method-level generics + lambda
// @Script

interface Converter {
    function convert<T, R>(input: T, transformer: (T) -> R) : R;
    function convertArray<T, R>(inputs: T[], transformer: (T) -> R) : R[];
}

class GenericConverter implements Converter {
    public function convert<T, R>(input: T, transformer: (T) -> R) : R {
        return transformer(input);
    }

    public function convertArray<T, R>(inputs: T[], transformer: (T) -> R) : R[] {
        R[] results = [];
        Int i = 0;
        while (i < inputs.length()) {
            results.push(transformer(inputs[i]));
            i = i + 1;
        }
        return results;
    }
}

function main() : void {
    Converter converter = new GenericConverter();

    // Convert single Int to String
    String str = converter.convert<Int, String>(
        42,
        (n: Int) : String => { return "Number: " + n.toString(); }
    );
    print(str);
    assert(str == "Number: 42", "Should convert Int to String");

    // Convert String to Int
    Int len = converter.convert<String, Int>(
        "Hello",
        (s: String) : Int => { return s.length(); }
    );
    print("Length: " + len.toString());
    assert(len == 5, "Should convert String to Int");

    // Convert array of Int to array of String
    Int[] numbers = [1, 2, 3];
    String[] strings = converter.convertArray<Int, String>(
        numbers,
        (n: Int) : String => { return "Item-" + n.toString(); }
    );

    print("First: " + strings[0]);
    assert(strings[0] == "Item-1", "Should convert array element");
    assert(strings.length() == 3, "Should have same length");

    print("Method-level generics test passed");
}
