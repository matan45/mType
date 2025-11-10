import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Test collection literal type inference

class IntArray {
    int[] items;
    int size;

    constructor(int length) {
        items = new int[length];
        size = 0;
    }

    public function add(Int value): void {
        items[size] = value.getValue();
        size = size + 1;
    }

    public function get(int index): Int {
        return new Int(items[index]);
    }

    public function getSize(): int {
        return size;
    }
}

class StringArray {
    string[] items;
    int size;

    constructor(int length) {
        items = new string[length];
        size = 0;
    }

    public function add(String value): void {
        items[size] = value.getValue();
        size = size + 1;
    }

    public function get(int index): String {
        return new String(items[index]);
    }

    public function getSize(): int {
        return size;
    }
}

// Function that creates and returns an array
function createIntArray(): IntArray {
    IntArray arr = new IntArray(5);
    arr.add(new Int(1));
    arr.add(new Int(2));
    arr.add(new Int(3));
    arr.add(new Int(4));
    arr.add(new Int(5));
    return arr;
}

function createStringArray(): StringArray {
    StringArray arr = new StringArray(3);
    arr.add(new String("hello"));
    arr.add(new String("world"));
    arr.add(new String("mType"));
    return arr;
}

// Function that processes arrays
function sumArray(IntArray arr): Int {
    int total = 0;
    int i = 0;
    while (i < arr.getSize()) {
        total = total + arr.get(i).getValue();
        i = i + 1;
    }
    return new Int(total);
}

function concatenateArray(StringArray arr): String {
    string result = "";
    int i = 0;
    while (i < arr.getSize()) {
        if (i > 0) {
            result = result + " ";
        }
        result = result + arr.get(i).getValue();
        i = i + 1;
    }
    return new String(result);
}

function main(): void {
    print("Testing collection literal type inference");

    // Test Int array literal inference
    IntArray intArr = createIntArray();
    print("IntArray size: " + new Int(intArr.getSize()));
    print("IntArray[0]: " + intArr.get(0));
    print("IntArray[2]: " + intArr.get(2));
    print("IntArray[4]: " + intArr.get(4));

    // Test sum of Int array
    Int sum = sumArray(intArr);
    print("Sum of IntArray: " + sum);

    // Test String array literal inference
    StringArray strArr = createStringArray();
    print("StringArray size: " + new Int(strArr.getSize()));
    print("StringArray[0]: " + strArr.get(0));
    print("StringArray[1]: " + strArr.get(1));
    print("StringArray[2]: " + strArr.get(2));

    // Test concatenation of String array
    String concat = concatenateArray(strArr);
    print("Concatenated StringArray: " + concat);

    // Test inline array creation and usage
    IntArray arr2 = new IntArray(3);
    arr2.add(new Int(10));
    arr2.add(new Int(20));
    arr2.add(new Int(30));
    Int sum2 = sumArray(arr2);
    print("Sum of inline IntArray: " + sum2);

    // Test empty array
    IntArray emptyArr = new IntArray(0);
    print("Empty array size: " + new Int(emptyArr.getSize()));
    Int emptySum = sumArray(emptyArr);
    print("Sum of empty array: " + emptySum);

    print("Collection literal type inference test completed");
}

main();
