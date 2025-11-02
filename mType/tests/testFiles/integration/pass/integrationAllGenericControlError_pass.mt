// Test: Generics with control flow and error handling integration
// @Script

interface Comparable<T> {
    compareTo(other: T) : Int;
}

class NumberWrapper<T> : Comparable<NumberWrapper<T>> {
    private value: T;

    constructor(v: T) {
        this.value = v;
    }

    getValue() : T {
        return this.value;
    }

    setValue(v: T) : Void {
        this.value = v;
    }

    compareTo(other: NumberWrapper<T>) : Int {
        // This will be overridden in specialized versions
        return 0;
    }

    toString() : String {
        return this.value.toString();
    }
}

class IntWrapper extends NumberWrapper<Int> {
    constructor(v: Int) {
        super(v);
    }

    compareTo(other: NumberWrapper<Int>) : Int {
        let otherVal: Int = other.getValue();
        let myVal: Int = this.getValue();

        if (myVal > otherVal) {
            return 1;
        } else if (myVal < otherVal) {
            return -1;
        }
        return 0;
    }

    add(other: IntWrapper) : IntWrapper {
        let result: Int = this.getValue() + other.getValue();
        return new IntWrapper(result);
    }
}

class FloatWrapper extends NumberWrapper<Float> {
    constructor(v: Float) {
        super(v);
    }

    compareTo(other: NumberWrapper<Float>) : Int {
        let otherVal: Float = other.getValue();
        let myVal: Float = this.getValue();

        if (myVal > otherVal) {
            return 1;
        } else if (myVal < otherVal) {
            return -1;
        }
        return 0;
    }

    multiply(other: FloatWrapper) : FloatWrapper {
        let result: Float = this.getValue() * other.getValue();
        return new FloatWrapper(result);
    }
}

class GenericContainer<T> {
    private items: T[];
    private capacity: Int;
    private size: Int;

    constructor(cap: Int) {
        if (cap <= 0) {
            throw "Capacity must be positive";
        }
        this.capacity = cap;
        this.items = new T[cap];
        this.size = 0;
    }

    add(item: T) : Bool {
        if (this.size >= this.capacity) {
            throw "Container is full";
        }
        this.items[this.size] = item;
        this.size = this.size + 1;
        return true;
    }

    get(index: Int) : T {
        if (index < 0 || index >= this.size) {
            throw "Index out of bounds: " + index.toString();
        }
        return this.items[index];
    }

    getSize() : Int {
        return this.size;
    }

    findMax<U extends Comparable<U>>(items: U[]) : U? {
        if (items.length() == 0) {
            return null;
        }

        let max: U = items[0];
        let i: Int = 1;

        while (i < items.length()) {
            try {
                if (items[i].compareTo(max) > 0) {
                    max = items[i];
                }
            } catch (e: String) {
                print("Error comparing at index " + i.toString() + ": " + e);
            }
            i = i + 1;
        }

        return max;
    }

    processBatch<U>(processor: (T) -> U) : U[] {
        let results: U[] = new U[this.size];
        let i: Int = 0;

        while (i < this.size) {
            try {
                results[i] = processor(this.items[i]);
            } catch (e: String) {
                print("Processing error at index " + i.toString() + ": " + e);
                throw "Batch processing failed";
            }
            i = i + 1;
        }

        return results;
    }
}

safeDivide(a: Int, b: Int) : Int {
    if (b == 0) {
        throw "Division by zero";
    }
    return a / b;
}

processNumbers<T extends NumberWrapper<Int>>(numbers: T[], operation: String) : Int {
    let result: Int = 0;
    let i: Int = 0;
    let errorCount: Int = 0;

    while (i < numbers.length()) {
        try {
            let num: T = numbers[i];
            let val: Int = num.getValue();

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
                    } catch (e: String) {
                        print("Division error: " + e);
                        errorCount = errorCount + 1;
                    }
                }
            } else {
                throw "Unknown operation: " + operation;
            }

        } catch (e: String) {
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

sortGeneric<T extends Comparable<T>>(items: T[]) : Void {
    // Bubble sort with error handling
    let n: Int = items.length();
    let i: Int = 0;

    while (i < n - 1) {
        let j: Int = 0;
        while (j < n - i - 1) {
            try {
                if (items[j].compareTo(items[j + 1]) > 0) {
                    // Swap
                    let temp: T = items[j];
                    items[j] = items[j + 1];
                    items[j + 1] = temp;
                }
            } catch (e: String) {
                print("Sort error at position " + j.toString() + ": " + e);
                throw "Sorting failed";
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

main() : Void {
    print("=== Generic Control Error Test ===");

    // Test 1: Generic container with control flow
    print("\n--- Test 1: Generic Container ---");
    try {
        let container: GenericContainer<IntWrapper> = new GenericContainer<IntWrapper>(5);

        let i: Int = 0;
        while (i < 5) {
            container.add(new IntWrapper(i * 10));
            i = i + 1;
        }

        print("Container size: " + container.getSize().toString());

        // Try to add beyond capacity
        try {
            container.add(new IntWrapper(99));
            print("Should not reach here");
        } catch (e: String) {
            print("Caught expected error: " + e);
        }

    } catch (e: String) {
        print("Container error: " + e);
    }

    // Test 2: Finding max with generics
    print("\n--- Test 2: Find Max ---");
    let numbers: IntWrapper[] = new IntWrapper[5];
    numbers[0] = new IntWrapper(42);
    numbers[1] = new IntWrapper(17);
    numbers[2] = new IntWrapper(99);
    numbers[3] = new IntWrapper(33);
    numbers[4] = new IntWrapper(8);

    let container2: GenericContainer<IntWrapper> = new GenericContainer<IntWrapper>(1);
    let maxNum: IntWrapper? = container2.findMax<IntWrapper>(numbers);
    if (maxNum != null) {
        print("Max value: " + maxNum.getValue().toString());
    }

    // Test 3: Sorting with type checking
    print("\n--- Test 3: Sorting ---");
    try {
        sortGeneric<IntWrapper>(numbers);
        print("Sorted values:");
        let idx: Int = 0;
        while (idx < numbers.length()) {
            print("  " + numbers[idx].getValue().toString());
            idx = idx + 1;
        }
    } catch (e: String) {
        print("Sort failed: " + e);
    }

    // Test 4: Arithmetic operations with error handling
    print("\n--- Test 4: Operations ---");
    let sum: Int = processNumbers<IntWrapper>(numbers, "sum");
    print("Sum: " + sum.toString());

    let product: Int = processNumbers<IntWrapper>(numbers, "product");
    print("Product: " + product.toString());

    // Test division with zero
    let divNums: IntWrapper[] = new IntWrapper[3];
    divNums[0] = new IntWrapper(100);
    divNums[1] = new IntWrapper(10);
    divNums[2] = new IntWrapper(0);

    let divResult: Int = processNumbers<IntWrapper>(divNums, "divide");
    print("Division result: " + divResult.toString());

    // Test 5: Float operations
    print("\n--- Test 5: Float Operations ---");
    let floats: FloatWrapper[] = new FloatWrapper[3];
    floats[0] = new FloatWrapper(3.14);
    floats[1] = new FloatWrapper(2.0);
    floats[2] = new FloatWrapper(1.5);

    let container3: GenericContainer<FloatWrapper> = new GenericContainer<FloatWrapper>(1);
    let maxFloat: FloatWrapper? = container3.findMax<FloatWrapper>(floats);
    if (maxFloat != null) {
        print("Max float: " + maxFloat.getValue().toString());
    }

    print("\n=== All tests completed ===");
}
