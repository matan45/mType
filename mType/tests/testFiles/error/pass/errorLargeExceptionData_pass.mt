// Test: Exception with large data payload
// Expected: Should handle exceptions with substantial data efficiently
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/collections/List.mt";

// Exception with large string payload
class LargeDataException extends Exception {
    public String largeData;
    public List<String> dataList;
    public Int dataSize;

    public constructor(string msg, String data, List<String> list, Int size): super(msg) {
        largeData = data;
        dataList = list;
        dataSize = size;
    }

    public function getDataSize(): Int {
        return dataSize;
    }

    public function getListSize(): Int {
        return dataList.size();
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

// Build a large list of strings
function buildLargeList(Int size): List<String> {
    List<String> list = new List<String>();
    Int i = new Int(0);

    while (i < size) {
        list.add(new String("Item_" + i));
        i = i + 1;
    }

    return list;
}

// Test exception with large string data
function testLargeStringException(): void {
    print("Creating exception with large string (100 elements):");
    try {
        int size = 100;
        String largeStr = buildLargeString(size);
        List<String> emptyList = new List<String>();

        throw new LargeDataException("Large string payload", largeStr, emptyList, size);
    } catch (LargeDataException e) {
        print("Caught exception with data size: " + e.getDataSize());
        print("Exception message: " + e.getMessage());
    }
}

// Test exception with large list data
function testLargeListException(): void {
    print("Creating exception with large list (200 elements):");
    try {
        Int size = new Int(200);
        String emptyStr = new String("");
        List<String> largeList = buildLargeList(size);

        throw new LargeDataException("Large list payload", emptyStr, largeList, size);
    } catch (LargeDataException e) {
        print("Caught exception with list size: " + e.getListSize());
        print("Exception message: " + e.getMessage());
    }
}

// Test exception with both large string and list
function testCombinedLargeDataException(): void {
    print("Creating exception with both large string and list:");
    try {
        int strSize = 50;
        int listSize = 100;
        String largeStr = buildLargeString(strSize);
        List<String> largeList = buildLargeList(listSize);

        throw new LargeDataException("Combined large payload", largeStr, largeList, strSize + listSize);
    } catch (LargeDataException e) {
        print("Caught exception with combined data");
        print("Data size: " + e.getDataSize());
        print("List size: " + e.getListSize());
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
            List<String> list = buildLargeList(size);

            throw new LargeDataException("Multiple large exception " + i, data, list, size);
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
            List<String> list = buildLargeList(size);

            throw new LargeDataException("Inner large exception", data, list, size);
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
testLargeListException();
print("---");
testCombinedLargeDataException();
print("---");
testMultipleLargeExceptions();
print("---");
testNestedLargeException();
print("---");
print("Large exception data test completed successfully!");
