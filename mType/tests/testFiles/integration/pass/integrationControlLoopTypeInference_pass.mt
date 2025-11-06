// Test: Type inference in various loop constructs
// Tests type inference with for loops, while loops, and loop variables

// Test 1: Type inference in for loops with arrays
print("Test 1: Type inference in for loops");

Int[] numbers = new Int[5];
Int i = 0;
while (i < 5) {
    numbers[i] = i * 10;
    i = i + 1;
}

i = 0;
while (i < 5) {
    Int num = numbers[i];
    print("numbers[" + i + "] = " + num);
    i = i + 1;
}

// Test 2: Type inference with loop accumulation
print("\nTest 2: Type inference with accumulation");

function sumArray(Int[] arr, Int size): Int {
    Int sum = 0;
    Int index = 0;
    while (index < size) {
        sum = sum + arr[index];
        index = index + 1;
    }
    return sum;
}

Int total = sumArray(numbers, 5);
print("Sum of array: " + total);

// Test 3: Type inference in nested loops
print("\nTest 3: Type inference in nested loops");

class Matrix {
    private Int[][] data;
    private Int rows;
    private Int cols;

    public constructor(Int r, Int c) {
        this.rows = r;
        this.cols = c;
        this.data = new Int[r][c];
    }

    public void fill(Int value) {
        Int i = 0;
        while (i < this.rows) {
            Int j = 0;
            while (j < this.cols) {
                this.data[i][j] = value + i + j;
                j = j + 1;
            }
            i = i + 1;
        }
    }

    public void print() {
        Int i = 0;
        while (i < this.rows) {
            String row = "";
            Int j = 0;
            while (j < this.cols) {
                Int val = this.data[i][j];
                row = row + val + " ";
                j = j + 1;
            }
            print(row);
            i = i + 1;
        }
    }

    public Int sum(): Int {
        Int total = 0;
        Int i = 0;
        while (i < this.rows) {
            Int j = 0;
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

function findFirst(Int[] arr, Int size, Int target): Int {
    Int index = 0;
    while (index < size) {
        Int current = arr[index];
        if (current == target) {
            return index;
        }
        index = index + 1;
    }
    return -1;
}

Int[] searchArray = new Int[10];
i = 0;
while (i < 10) {
    searchArray[i] = i * 2;
    i = i + 1;
}

Int found = findFirst(searchArray, 10, 8);
print("Found 8 at index: " + found);

found = findFirst(searchArray, 10, 15);
print("Found 15 at index: " + found);

// Test 5: Type inference with loop variable transformations
print("\nTest 5: Type inference with transformations");

class StringBuffer {
    private String[] parts;
    private Int count;
    private Int capacity;

    public constructor(Int cap) {
        this.capacity = cap;
        this.parts = new String[cap];
        this.count = 0;
    }

    public void append(String s) {
        if (this.count < this.capacity) {
            this.parts[this.count] = s;
            this.count = this.count + 1;
        }
    }

    public String toString(): String {
        String result = "";
        Int i = 0;
        while (i < this.count) {
            String part = this.parts[i];
            result = result + part;
            i = i + 1;
        }
        return result;
    }
}

StringBuffer buffer = new StringBuffer(5);
Int counter = 0;
while (counter < 5) {
    String part = "Part" + counter;
    buffer.append(part);
    counter = counter + 1;
}

print("Buffer contents: " + buffer.toString());

// Test 6: Type inference with loop-scoped variables
print("\nTest 6: Type inference with loop scopes");

function processNumbers(Int[] nums, Int size): String[] {
    String[] results = new String[size];
    Int idx = 0;
    while (idx < size) {
        Int num = nums[idx];
        String result;
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

Int[] testNums = new Int[5];
i = 0;
while (i < 5) {
    testNums[i] = i + 1;
    i = i + 1;
}

String[] processed = processNumbers(testNums, 5);
i = 0;
while (i < 5) {
    print(testNums[i] + " is " + processed[i]);
    i = i + 1;
}

// Test 7: Type inference with complex loop expressions
print("\nTest 7: Complex loop expressions");

class Accumulator {
    private Int value;

    public constructor() {
        this.value = 0;
    }

    public void add(Int v) {
        this.value = this.value + v;
    }

    public Int get(): Int {
        return this.value;
    }
}

Accumulator acc = new Accumulator();
i = 1;
while (i <= 10) {
    Int square = i * i;
    acc.add(square);
    i = i + 1;
}

print("Sum of squares 1-10: " + acc.get());

print("\nLoop type inference tests completed");
