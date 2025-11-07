import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Array type inference with generics
function <T> createArray(T first, T second, T third): T[] {
    T[] arr = new T[3];
    arr[0] = first;
    arr[1] = second;
    arr[2] = third;
    return arr;
}

function <T> printArray(T[] array, Int size): void {
    Int i = new Int(0);
    while (i.getValue() < size.getValue()) {
        print(array[i.getValue()].toString());
        i = new Int(i.getValue() + 1);
    }
}

function main(): void {
    // Infer T as String from array creation
    String[] strings = createArray<String>(new String("A"), new String("B"), new String("C"));
    printArray<String>(strings, new Int(3));

    // Infer T as Int from array creation
    Int[] numbers = createArray<Int>(new Int(1), new Int(2), new Int(3));
    printArray<Int>(numbers, new Int(3));
}

main();
