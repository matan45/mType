import * from "../../lib/primitives/Int.mt";

// Test raw type to generic cast
class Box<T> {
    T content;

    public function setContent(T item): void {
        content = item;
    }

    public function getContent(): T {
        return content;
    }
}

// Function that returns raw Box (without type parameter)
function createRawBox(): Box {
    Box rawBox = new Box();
    return rawBox;
}

// Function that works with generic Box
function useGenericBox(Box<Int> intBox): void {
    intBox.setContent(new Int(999));
    Int value = intBox.getContent();
    print("Generic box value: " + value);
}

function main(): void {
    // Create raw Box
    Box rawBox = new Box();
    print("Raw box created");

    // Cast raw Box to generic Box<Int>
    Box<Int> intBox = rawBox as Box<Int>;
    intBox.setContent(new Int(42));
    Int value = intBox.getContent();
    print("Cast value: " + value);

    // Get raw box from function and cast
    Box rawFromFunction = createRawBox();
    Box<Int> genericFromRaw = rawFromFunction as Box<Int>;
    useGenericBox(genericFromRaw);

    // Create generic and cast back to raw
    Box<Int> specificBox = new Box<Int>();
    specificBox.setContent(new Int(100));
    Box backToRaw = specificBox as Box;
    print("Back to raw successful");

    print("Raw type to generic cast successful");
}

main();
