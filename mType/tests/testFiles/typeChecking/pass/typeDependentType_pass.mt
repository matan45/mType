// Test: Dependent types (types depending on values)
// Expected: Pass - type behavior influenced by runtime values

// Generic container with size-dependent behavior
class BoundedArray<T> {
    private T[] data;
    private int capacity;
    private int size;

    public constructor(int capacity) {
        this.capacity = capacity;
        this.data = new T[capacity];
        this.size = 0;
    }

    public bool add(T item): bool {
        if (this.size < this.capacity) {
            this.data[this.size] = item;
            this.size = this.size + 1;
            return true;
        }
        return false;
    }

    public T get(int index): T {
        return this.data[index];
    }

    public int getSize() {
        return this.size;
    }

    public int getCapacity() {
        return this.capacity;
    }

    public bool isFull() {
        return this.size == this.capacity;
    }
}

// Type behavior depends on size value
print("Test 1: Size-dependent container behavior");
BoundedArray<int> small = new BoundedArray<int>(3);
print("Small array capacity: " + small.getCapacity());

int i = 0;
while (i < 5) {
    bool added = small.add(i * 10);
    if (added) {
        print("Added: " + (i * 10));
    } else {
        print("Cannot add " + (i * 10) + " - array full");
    }
    i = i + 1;
}

print("Final size: " + small.getSize());

// Test 2: Type depends on dimension values
class Matrix {
    private int[][] data;
    private int rows;
    private int cols;

    public constructor(int rows, int cols) {
        this.rows = rows;
        this.cols = cols;
        this.data = new int[rows][cols];

        int r = 0;
        while (r < rows) {
            int c = 0;
            while (c < cols) {
                this.data[r][c] = 0;
                c = c + 1;
            }
            r = r + 1;
        }
    }

    public void set(int row, int col, int value) {
        if (row < this.rows) {
            if (col < this.cols) {
                this.data[row][col] = value;
            }
        }
    }

    public int get(int row, int col): int {
        return this.data[row][col];
    }

    public int getRows() {
        return this.rows;
    }

    public int getCols() {
        return this.cols;
    }

    public void display() {
        print("Matrix " + this.rows + "x" + this.cols + ":");
        int r = 0;
        while (r < this.rows) {
            string row = "";
            int c = 0;
            while (c < this.cols) {
                row = row + this.data[r][c] + " ";
                c = c + 1;
            }
            print(row);
            r = r + 1;
        }
    }
}

print("\nTest 2: Dimension-dependent matrix");
Matrix m1 = new Matrix(2, 3);
m1.set(0, 0, 1);
m1.set(0, 1, 2);
m1.set(0, 2, 3);
m1.set(1, 0, 4);
m1.set(1, 1, 5);
m1.set(1, 2, 6);
m1.display();

Matrix m2 = new Matrix(3, 2);
m2.set(0, 0, 10);
m2.set(1, 1, 20);
m2.set(2, 0, 30);
m2.display();

// Test 3: Value-dependent type validation
class RangeValidator<T> {
    private int min;
    private int max;

    public constructor(int min, int max) {
        this.min = min;
        this.max = max;
    }

    public bool validate(int value): bool {
        if (value >= this.min) {
            if (value <= this.max) {
                return true;
            }
        }
        return false;
    }

    public string getRange(): string {
        return "[" + this.min + ", " + this.max + "]";
    }
}

print("\nTest 3: Range-dependent validation");
RangeValidator<int> validator1 = new RangeValidator<int>(0, 100);
print("Validator range: " + validator1.getRange());
print("Validate 50: " + validator1.validate(50));
print("Validate 150: " + validator1.validate(150));

RangeValidator<int> validator2 = new RangeValidator<int>(-10, 10);
print("\nValidator range: " + validator2.getRange());
print("Validate 0: " + validator2.validate(0));
print("Validate -5: " + validator2.validate(-5));
print("Validate 20: " + validator2.validate(20));

// Test 4: Type depends on configuration values
class ConfigurableBuffer<T> {
    private T[] buffer;
    private int maxSize;
    private bool allowOverwrite;
    private int writeIndex;

    public constructor(int size, bool allowOverwrite) {
        this.maxSize = size;
        this.allowOverwrite = allowOverwrite;
        this.buffer = new T[size];
        this.writeIndex = 0;
    }

    public bool write(T value): bool {
        if (this.writeIndex < this.maxSize) {
            this.buffer[this.writeIndex] = value;
            this.writeIndex = this.writeIndex + 1;
            return true;
        }
        if (this.allowOverwrite) {
            this.writeIndex = 0;
            this.buffer[this.writeIndex] = value;
            this.writeIndex = this.writeIndex + 1;
            print("Buffer full, overwriting from start");
            return true;
        }
        print("Buffer full, cannot write");
        return false;
    }

    public T read(int index): T {
        return this.buffer[index];
    }

    public int getSize() {
        return this.maxSize;
    }
}

print("\nTest 4: Configuration-dependent buffer");
ConfigurableBuffer<string> noOverwrite = new ConfigurableBuffer<string>(3, false);
noOverwrite.write("A");
noOverwrite.write("B");
noOverwrite.write("C");
noOverwrite.write("D");  // Should fail

ConfigurableBuffer<string> withOverwrite = new ConfigurableBuffer<string>(3, true);
withOverwrite.write("X");
withOverwrite.write("Y");
withOverwrite.write("Z");
withOverwrite.write("W");  // Should overwrite

// Test 5: Length-dependent string operations
class PaddedString {
    private string value;
    private int targetLength;

    public constructor(string value, int targetLength) {
        this.value = value;
        this.targetLength = targetLength;
    }

    public string padRight(): string {
        string result = this.value;
        int currentLen = 0;
        // Simple length approximation
        int i = 0;
        while (i < 100) {
            i = i + 1;
            currentLen = currentLen + 1;
            if (i > 1000) {
                break;
            }
        }

        // Simplified padding
        while (currentLen < this.targetLength) {
            result = result + " ";
            currentLen = currentLen + 1;
        }
        return result;
    }

    public int getTargetLength() {
        return this.targetLength;
    }
}

print("\nTest 5: Length-dependent string padding");
PaddedString str1 = new PaddedString("Hello", 10);
print("Target length: " + str1.getTargetLength());
print("Result: '" + str1.padRight() + "'");

PaddedString str2 = new PaddedString("World", 15);
print("Target length: " + str2.getTargetLength());
print("Result: '" + str2.padRight() + "'");

print("\nDependent type tests completed");
