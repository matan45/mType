// Test: Generics with control flow and error handling integration
// @Script

interface Comparable<T> {
    function compareTo(T other) : Int;
}

class NumberWrapper<T> implements Comparable<NumberWrapper<T>> {
    private T value;

    constructor(T v) {
        this.value = v;
    }

    function getValue() : T {
        return this.value;
    }

    function setValue(T v) : void {
        this.value = v;
    }

    function compareTo(NumberWrapper<T> other) : Int {
        // This will be overridden in specialized versions
        return 0;
    }

    function toString() : String {
        return this.value.toString();
    }
}

class IntWrapper extends NumberWrapper<Int> {
    constructor(Int v) {
        super(v);
    }

    function compareTo(NumberWrapper<Int> other) : Int {
        Int otherVal = other.getValue();
        Int myVal = this.getValue();

        if (myVal > otherVal) {
            return 1;
        } else if (myVal < otherVal) {
            return -1;
        }
        return 0;
    }

    function add(IntWrapper other) : IntWrapper {
        Int result = this.getValue() + other.getValue();
        return new IntWrapper(result);
    }
}

class FloatWrapper extends NumberWrapper<Float> {
    constructor(Float v) {
        super(v);
    }

    function compareTo(NumberWrapper<Float> other) : Int {
        Float otherVal = other.getValue();
        Float myVal = this.getValue();

        if (myVal > otherVal) {
            return 1;
        } else if (myVal < otherVal) {
            return -1;
        }
        return 0;
    }

    function multiply(FloatWrapper other) : FloatWrapper {
        Float result = this.getValue() * other.getValue();
        return new FloatWrapper(result);
    }
}

class GenericContainer<T> {
    private T[] items;
    private Int capacity;
    private Int size;

    constructor(Int cap) {
        if (cap <= 0) {
            throw "Capacity must be positive";
        }
        this.capacity = cap;
        this.items = new T[cap];
        this.size = 0;
    }

    function add(T item) : Bool {
        if (this.size >= this.capacity) {
            throw "Container is full";
        }
        this.items[this.size] = item;
        this.size = this.size + 1;
        return true;
    }

    function get(Int index) : T {
        if (index < 0 || index >= this.size) {
            throw "Index out of bounds: " + index.toString();
        }
        return this.items[index];
    }

    function getSize() : Int {
        return this.size;
    }

    function findMax<U extends Comparable<U>>(U[] items) : U? {
        if (items.length() == 0) {
            return null;
        }

        U max = items[0];
        Int i = 1;

        while (i < items.length()) {
            try {
                if (items[i].compareTo(max) > 0) {
                    max = items[i];
                }
            } catch (String e) {
                print("Error comparing at index " + i.toString() + ": " + e);
            }
            i = i + 1;
        }

        return max;
    }

    function processBatch<U>((T) -> U processor) : U[] {
        U[] results = new U[this.size];
        Int i = 0;

        while (i < this.size) {
            try {
                results[i] = processor(this.items[i]);
            } catch (String e) {
                print("Processing error at index " + i.toString() + ": " + e);
                throw "Batch processing failed";
            }
            i = i + 1;
        }

        return results;
    }
}

function safeDivide(Int a, Int b) : Int {
    if (b == 0) {
        throw "Division by zero";
    }
    return a / b;
}

function processNumbers<T extends NumberWrapper<Int>>(T[] numbers, String operation) : Int {
    Int result = 0;
    Int i = 0;
    Int errorCount = 0;

    while (i < numbers.length()) {
        try {
            T num = numbers[i];
            Int val = num.getValue();

            if (operation == "sum") {
                result = result + val;
            } else if (operation == "product") {
                if (i == 0) {
                    result = val;
                } else {
                    result = result * val;
                }
            } else if (operation == "divide") {
                if (i == 0) {
                    result = val;
                } else {
                    try {
                        result = safeDivide(result, val);
                    } catch (String e) {
                        print("Division error: " + e);
                        errorCount = errorCount + 1;
                    }
                }
            } else {
                throw "Unknown operation: " + operation;
            }

        } catch (String e) {
            print("Error processing number at index " + i.toString() + ": " + e);
            errorCount = errorCount + 1;
        }

        i = i + 1;
    }

    if (errorCount > 0) {
        print("Completed with " + errorCount.toString() + " errors");
    }

    return result;
}

function sortGeneric<T extends Comparable<T>>(T[] items) : void {
    // Bubble sort with error handling
    Int n = items.length();
    Int i = 0;

    while (i < n - 1) {
        Int j = 0;
        while (j < n - i - 1) {
            try {
                if (items[j].compareTo(items[j + 1]) > 0) {
                    // Swap
                    T temp = items[j];
                    items[j] = items[j + 1];
                    items[j + 1] = temp;
                }
            } catch (String e) {
                print("Sort error at position " + j.toString() + ": " + e);
                throw "Sorting failed";
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

function main() : void {
    print("=== Generic Control Error Test ===");

    // Test 1: Generic container with control flow
    print("\n--- Test 1: Generic Container ---");
    try {
        GenericContainer<IntWrapper> container = new GenericContainer<IntWrapper>(5);

        Int i = 0;
        while (i < 5) {
            container.add(new IntWrapper(i * 10));
            i = i + 1;
        }

        print("Container size: " + container.getSize().toString());

        // Try to add beyond capacity
        try {
            container.add(new IntWrapper(99));
            print("Should not reach here");
        } catch (String e) {
            print("Caught expected error: " + e);
        }

    } catch (String e) {
        print("Container error: " + e);
    }

    // Test 2: Finding max with generics
    print("\n--- Test 2: Find Max ---");
    IntWrapper[] numbers = new IntWrapper[5];
    numbers[0] = new IntWrapper(42);
    numbers[1] = new IntWrapper(17);
    numbers[2] = new IntWrapper(99);
    numbers[3] = new IntWrapper(33);
    numbers[4] = new IntWrapper(8);

    GenericContainer<IntWrapper> container2 = new GenericContainer<IntWrapper>(1);
    IntWrapper? maxNum = container2.findMax<IntWrapper>(numbers);
    if (maxNum != null) {
        print("Max value: " + maxNum.getValue().toString());
    }

    // Test 3: Sorting with type checking
    print("\n--- Test 3: Sorting ---");
    try {
        sortGeneric<IntWrapper>(numbers);
        print("Sorted values:");
        Int idx = 0;
        while (idx < numbers.length()) {
            print("  " + numbers[idx].getValue().toString());
            idx = idx + 1;
        }
    } catch (String e) {
        print("Sort failed: " + e);
    }

    // Test 4: Arithmetic operations with error handling
    print("\n--- Test 4: Operations ---");
    Int sum = processNumbers<IntWrapper>(numbers, "sum");
    print("Sum: " + sum.toString());

    Int product = processNumbers<IntWrapper>(numbers, "product");
    print("Product: " + product.toString());

    // Test division with zero
    IntWrapper[] divNums = new IntWrapper[3];
    divNums[0] = new IntWrapper(100);
    divNums[1] = new IntWrapper(10);
    divNums[2] = new IntWrapper(0);

    Int divResult = processNumbers<IntWrapper>(divNums, "divide");
    print("Division result: " + divResult.toString());

    // Test 5: Float operations
    print("\n--- Test 5: Float Operations ---");
    FloatWrapper[] floats = new FloatWrapper[3];
    floats[0] = new FloatWrapper(3.14);
    floats[1] = new FloatWrapper(2.0);
    floats[2] = new FloatWrapper(1.5);

    GenericContainer<FloatWrapper> container3 = new GenericContainer<FloatWrapper>(1);
    FloatWrapper? maxFloat = container3.findMax<FloatWrapper>(floats);
    if (maxFloat != null) {
        print("Max Float: " + maxFloat.getValue().toString());
    }

    print("\n=== All tests completed ===");
}
