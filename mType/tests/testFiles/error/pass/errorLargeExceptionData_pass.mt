// Test: Exception with large data payload
// Expected: Should handle exceptions with substantial data efficiently
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/collections/ArrayList.mt";

// Exception with large string payload
class LargeDataException extends Exception {
    public String largeData;
    public ArrayList<String> dataArrayList;
    public Int dataSize;

    public constructor(string msg, String data, ArrayList<String> ArrayList, Int size): super(msg) {
        largeData = data;
        dataArrayList = ArrayList;
        dataSize = size;
    }

    public function getDataSize(): Int {
        return dataSize;
    }

    public function getArrayListSize(): Int {
        return dataArrayList.size();
    }
}

// Build a large string by concatenation
function buildLargeString(int size): String {
    string result = "";
    int i = 0;

    while (i < size) {
        result = result + "Data" + i + "|";
        i = i + 1;
    }

    return result;
}

// Build a large ArrayList of strings
function buildLargeArrayList(Int size): ArrayList<String> {
    ArrayList<String> ArrayList = new ArrayList<String>();
    Int i = new Int(0);

    while (i < size) {
        ArrayList.add(new String("Item_" + i));
        i = i + 1;
    }

    return ArrayList;
}

// Test exception with large string data
function testLargeStringException(): void {
    print("Creating exception with large string (100 elements):");
    try {
        int size = 100;
        String largeStr = buildLargeString(size);
        ArrayList<String> emptyArrayList = new ArrayList<String>();

        throw new LargeDataException("Large string payload", largeStr, emptyArrayList, size);
    } catch (LargeDataException e) {
        print("Caught exception with data size: " + e.getDataSize());
        print("Exception message: " + e.getMessage());
    }
}

// Test exception with large ArrayList data
function testLargeArrayListException(): void {
    print("Creating exception with large ArrayList (200 elements):");
    try {
        Int size = new Int(200);
        String emptyStr = new String("");
        ArrayList<String> largeArrayList = buildLargeArrayList(size);

        throw new LargeDataException("Large ArrayList payload", emptyStr, largeArrayList, size);
    } catch (LargeDataException e) {
        print("Caught exception with ArrayList size: " + e.getArrayListSize());
        print("Exception message: " + e.getMessage());
    }
}

// Test exception with both large string and ArrayList
function testCombinedLargeDataException(): void {
    print("Creating exception with both large string and ArrayList:");
    try {
        int strSize = 50;
        int ArrayListSize = 100;
        String largeStr = buildLargeString(strSize);
        ArrayList<String> largeArrayList = buildLargeArrayList(ArrayListSize);

        throw new LargeDataException("Combined large payload", largeStr, largeArrayList, strSize + ArrayListSize);
    } catch (LargeDataException e) {
        print("Caught exception with combined data");
        print("Data size: " + e.getDataSize());
        print("ArrayList size: " + e.getArrayListSize());
    }
}

// Test multiple large exceptions in sequence
function testMultipleLargeExceptions(): void {
    print("Testing multiple large exceptions:");
    Int caught = new Int(0);

    Int i = new Int(0);
    Int iterations = new Int(10);
    while (i < iterations) {
        try {
            int size = 50;
            String data = buildLargeString(size);
            ArrayList<String> ArrayList = buildLargeArrayList(size);

            throw new LargeDataException("Multiple large exception " + i, data, ArrayList, size);
        } catch (LargeDataException e) {
            caught = caught + 1;
        }
        i = i + 1;
    }

    print("Caught large exceptions: " + caught);
}

// Nested exception with large data
class NestedLargeException extends Exception {
    public LargeDataException innerException;
    public String additionalData;

    public constructor(string msg, LargeDataException inner, String data): super(msg) {
        innerException = inner;
        additionalData = data;
    }

    public function getInnerDataSize(): Int {
        return innerException.getDataSize();
    }
}

function testNestedLargeException(): void {
    print("Testing nested exception with large data:");
    try {
        try {
            int size = 50;
            String data = buildLargeString(size);
            ArrayList<String> ArrayList = buildLargeArrayList(size);

            throw new LargeDataException("Inner large exception", data, ArrayList, size);
        } catch (LargeDataException inner) {
            String additionalData = buildLargeString(30);
            throw new NestedLargeException("Nested with large data", inner, additionalData);
        }
    } catch (NestedLargeException e) {
        print("Caught nested exception");
        print("Inner exception data size: " + e.getInnerDataSize());
    }
}

print("Starting large exception data performance test");
print("---");
testLargeStringException();
print("---");
testLargeArrayListException();
print("---");
testCombinedLargeDataException();
print("---");
testMultipleLargeExceptions();
print("---");
testNestedLargeException();
print("---");
print("Large exception data test completed successfully!");
