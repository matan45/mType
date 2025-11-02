// Test: Type inference in various loop constructs
// Tests type inference with for loops, while loops, and loop variables

// Test 1: Type inference in for loops with arrays
print("Test 1: Type inference in for loops");

int[] numbers = new int[5];
int i = 0;
while (i < 5) {
    numbers[i] = i * 10;
    i = i + 1;
}

i = 0;
while (i < 5) {
    int num = numbers[i];
    print("numbers[" + i + "] = " + num);
    i = i + 1;
}

// Test 2: Type inference with loop accumulation
print("\nTest 2: Type inference with accumulation");

function sumArray(int[] arr, int size): int {
    int sum = 0;
    int index = 0;
    while (index < size) {
        sum = sum + arr[index];
        index = index + 1;
    }
    return sum;
}

int total = sumArray(numbers, 5);
print("Sum of array: " + total);

// Test 3: Type inference with nested loops
print("\nTest 3: Type inference in nested loops");

class Matrix {
    private int[][] data;
    private int rows;
    private int cols;

    public constructor(int r, int c) {
        this.rows = r;
        this.cols = c;
        this.data = new int[r][c];
    }

    public void fill(int value) {
        int i = 0;
        while (i < this.rows) {
            int j = 0;
            while (j < this.cols) {
                this.data[i][j] = value + i + j;
                j = j + 1;
            }
            i = i + 1;
        }
    }

    public void print() {
        int i = 0;
        while (i < this.rows) {
            string row = "";
            int j = 0;
            while (j < this.cols) {
                int val = this.data[i][j];
                row = row + val + " ";
                j = j + 1;
            }
            print(row);
            i = i + 1;
        }
    }

    public int sum(): int {
        int total = 0;
        int i = 0;
        while (i < this.rows) {
            int j = 0;
            while (j < this.cols) {
                total = total + this.data[i][j];
                j = j + 1;
            }
            i = i + 1;
        }
        return total;
    }
}

Matrix matrix = new Matrix(3, 3);
matrix.fill(1);
matrix.print();
print("Matrix sum: " + matrix.sum());

// Test 4: Type inference with loop conditions and break/continue
print("\nTest 4: Type inference with break and continue");

function findFirst(int[] arr, int size, int target): int {
    int index = 0;
    while (index < size) {
        int current = arr[index];
        if (current == target) {
            return index;
        }
        index = index + 1;
    }
    return -1;
}

int[] searchArray = new int[10];
i = 0;
while (i < 10) {
    searchArray[i] = i * 2;
    i = i + 1;
}

int found = findFirst(searchArray, 10, 8);
print("Found 8 at index: " + found);

found = findFirst(searchArray, 10, 15);
print("Found 15 at index: " + found);

// Test 5: Type inference with loop variable transformations
print("\nTest 5: Type inference with transformations");

class StringBuffer {
    private string[] parts;
    private int count;
    private int capacity;

    public constructor(int cap) {
        this.capacity = cap;
        this.parts = new string[cap];
        this.count = 0;
    }

    public void append(string s) {
        if (this.count < this.capacity) {
            this.parts[this.count] = s;
            this.count = this.count + 1;
        }
    }

    public string toString(): string {
        string result = "";
        int i = 0;
        while (i < this.count) {
            string part = this.parts[i];
            result = result + part;
            i = i + 1;
        }
        return result;
    }
}

StringBuffer buffer = new StringBuffer(5);
int counter = 0;
while (counter < 5) {
    string part = "Part" + counter;
    buffer.append(part);
    counter = counter + 1;
}

print("Buffer contents: " + buffer.toString());

// Test 6: Type inference with loop-scoped variables
print("\nTest 6: Type inference with loop scopes");

function processNumbers(int[] nums, int size): string[] {
    string[] results = new string[size];
    int idx = 0;
    while (idx < size) {
        int num = nums[idx];
        string result;
        if (num % 2 == 0) {
            result = "even";
        } else {
            result = "odd";
        }
        results[idx] = result;
        idx = idx + 1;
    }
    return results;
}

int[] testNums = new int[5];
i = 0;
while (i < 5) {
    testNums[i] = i + 1;
    i = i + 1;
}

string[] processed = processNumbers(testNums, 5);
i = 0;
while (i < 5) {
    print(testNums[i] + " is " + processed[i]);
    i = i + 1;
}

// Test 7: Type inference with complex loop expressions
print("\nTest 7: Complex loop expressions");

class Accumulator {
    private int value;

    public constructor() {
        this.value = 0;
    }

    public void add(int v) {
        this.value = this.value + v;
    }

    public int get(): int {
        return this.value;
    }
}

Accumulator acc = new Accumulator();
i = 1;
while (i <= 10) {
    int square = i * i;
    acc.add(square);
    i = i + 1;
}

print("Sum of squares 1-10: " + acc.get());

print("\nLoop type inference tests completed");
