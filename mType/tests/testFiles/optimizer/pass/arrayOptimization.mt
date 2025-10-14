// Test file for array optimization
// Tests that the optimizer correctly handles arrays and index access

// ===== USED CODE (should be kept) =====

class ArrayProcessor {
    private int[] data;

    constructor(int size) {
        this.data = new int[size];
    }

    // Used method - accesses array
    public function set(int index, int value): void {
        this.data[index] = value;
    }

    // Used method - returns array element
    public function get(int index): int {
        return this.data[index];
    }

    // UNUSED method - should be removed
    public function clear(): void {
        for (int i = 0; i < this.data.length; i++) {
            this.data[i] = 0;
        }
    }

    // UNUSED method - should be removed
    public function sum(): int {
        int total = 0;
        for (int i = 0; i < this.data.length; i++) {
            total = total + this.data[i];
        }
        return total;
    }
}

// UNUSED class - should be removed
class UnusedArrayHelper {
    public static function reverse(int[] arr): int[] {
        int len = arr.length;
        int[] result = new int[len];
        for (int i = 0; i < len; i++) {
            result[i] = arr[len - 1 - i];
        }
        return result;
    }
}

// Used function
function processArray(): void {
    ArrayProcessor processor = new ArrayProcessor(5);
    processor.set(0, 10);
    processor.set(1, 20);

    int value = processor.get(0);
    print("Value at index 0: " + value);
}

// UNUSED function - should be removed
function unusedArrayFunction(): void {
    int[] arr = new int[3];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    print("Array created");
}

// ===== ENTRY POINT CODE =====

processArray();
print("Array optimization test complete!");
