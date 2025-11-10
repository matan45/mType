import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Generic instance methods with complex types (arrays)
class ArrayProcessor {
    public function <T> processArray(T[] arr, int size): void {
        print("Processing array of size: " + size);
        for (int i = 0; i < size; i = i + 1) {
            print("Element: " + arr[i]);
        }
    }

    public function <T> firstElement(T[] arr): T {
        return arr[0];
    }
}

function main(): void {
    ArrayProcessor processor = new ArrayProcessor();

    // Test with String array
    String[] strArray = new String[3];
    strArray[0] = new String("first");
    strArray[1] = new String("second");
    strArray[2] = new String("third");
    processor.processArray<String>(strArray, 3);

    String first = processor.firstElement<String>(strArray);
    print("First element: " + first);

    // Test with Int array
    Int[] intArray = new Int[2];
    intArray[0] = new Int(10);
    intArray[1] = new Int(20);
    processor.processArray<Int>(intArray, 2);

    Int firstInt = processor.firstElement<Int>(intArray);
    print("First int: " + firstInt);

    print("Complex types test passed");
}

main();
