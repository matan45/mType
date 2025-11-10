// Integration Test 08: Multi-Dimensional Arrays with Generics
// Tests: Multi-dimensional arrays + Generic classes + For-each simulation + Collections

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic matrix class
class Matrix<T> {
    private T[][] data;
    private int rows;
    private int cols;

    constructor(int r, int c) {
        this.rows = r;
        this.cols = c;
        this.data = new T[r][c];
    }

    public function set(int row, int col, T value): void {
        this.data[row][col] = value;
    }

    public function get(int row, int col): T {
        return this.data[row][col];
    }

    public function getRows(): int {
        return this.rows;
    }

    public function getCols(): int {
        return this.cols;
    }

    public function printAll(): void {
        for (int i = 0; i < this.rows; i = i + 1) {
            string row = "Row " + i + ": ";
            for (int j = 0; j < this.cols; j = j + 1) {
                T val = this.data[i][j];
                if (val != null) {
                    row = row + val + " ";
                } else {
                    row = row + "null ";
                }
            }
            print(row);
        }
    }
}

// Generic 3D tensor class
class Tensor<T> {
    private T[][][] data;
    private int depth;
    private int rows;
    private int cols;

    constructor(int d, int r, int c) {
        this.depth = d;
        this.rows = r;
        this.cols = c;
        this.data = new T[d][r][c];
    }

    public function set(int d, int r, int c, T value): void {
        this.data[d][r][c] = value;
    }

    public function get(int d, int r, int c): T {
        return this.data[d][r][c];
    }

    public function sumLayer(int layer): int {
        int sum = 0;
        for (int i = 0; i < this.rows; i = i + 1) {
            for (int j = 0; j < this.cols; j = j + 1) {
                T val = this.data[layer][i][j];
                if (val isClassOf Int) {
                    Int intVal = (Int)val;
                    sum = sum + intVal.getValue();
                }
            }
        }
        return sum;
    }
}

// Array processor with collections
class ArrayProcessor {
    public function processMatrix(int[][] matrix): List<Int> {
        List<Int> result = new List<Int>();

        for (int i = 0; i < matrix.length; i = i + 1) {
            for (int j = 0; j < matrix[i].length; j = j + 1) {
                int value = matrix[i][j];
                result.add(new Int(value * 2));
            }
        }

        return result;
    }

    public function flattenTo1D(int[][] matrix): int[] {
        int totalElements = 0;
        for (int i = 0; i < matrix.length; i = i + 1) {
            totalElements = totalElements + matrix[i].length;
        }

        int[] result = new int[totalElements];
        int index = 0;

        for (int i = 0; i < matrix.length; i = i + 1) {
            for (int j = 0; j < matrix[i].length; j = j + 1) {
                result[index] = matrix[i][j];
                index = index + 1;
            }
        }

        return result;
    }
}

// Test jagged arrays
function testJaggedArrays(): void {
    print("--- Jagged arrays ---");

    int[][] jagged = new int[3][];
    jagged[0] = [1, 2];
    jagged[1] = [3, 4, 5, 6];
    jagged[2] = [7];

    for (int i = 0; i < jagged.length; i = i + 1) {
        string row = "Jagged row " + i + " (length " + jagged[i].length + "): ";
        for (int j = 0; j < jagged[i].length; j = j + 1) {
            row = row + jagged[i][j] + " ";
        }
        print(row);
    }
}

// Test 3D arrays
function test3DArrays(): void {
    print("--- 3D arrays ---");

    int[][][] cube = new int[2][][];
    cube[0] = new int[2][];
    cube[1] = new int[2][];

    cube[0][0] = [1, 2];
    cube[0][1] = [3, 4];
    cube[1][0] = [5, 6];
    cube[1][1] = [7, 8];

    for (int d = 0; d < cube.length; d = d + 1) {
        print("Depth " + d + ":");
        for (int r = 0; r < cube[d].length; r = r + 1) {
            string row = "  Row " + r + ": ";
            for (int c = 0; c < cube[d][r].length; c = c + 1) {
                row = row + cube[d][r][c] + " ";
            }
            print(row);
        }
    }
}

// Main test execution
print("=== Test 08: Multi-Dimensional Arrays with Generics ===");

// Test 1: Basic 2D array
print("--- Basic 2D array ---");
int[][] matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];

for (int i = 0; i < matrix.length; i = i + 1) {
    string row = "Row " + i + ": ";
    for (int j = 0; j < matrix[i].length; j = j + 1) {
        row = row + matrix[i][j] + " ";
    }
    print(row);
}

// Test 2: Generic Matrix class
print("--- Generic Matrix with Int ---");
Matrix<Int> intMatrix = new Matrix<Int>(2, 3);
intMatrix.set(0, 0, new Int(10));
intMatrix.set(0, 1, new Int(20));
intMatrix.set(0, 2, new Int(30));
intMatrix.set(1, 0, new Int(40));
intMatrix.set(1, 1, new Int(50));
intMatrix.set(1, 2, new Int(60));

intMatrix.printAll();

// Test 3: Generic Matrix with String
print("--- Generic Matrix with String ---");
Matrix<String> strMatrix = new Matrix<String>(2, 2);
strMatrix.set(0, 0, new String("A"));
strMatrix.set(0, 1, new String("B"));
strMatrix.set(1, 0, new String("C"));
strMatrix.set(1, 1, new String("D"));

strMatrix.printAll();

// Test 4: 3D Tensor
print("--- 3D Tensor ---");
Tensor<Int> tensor = new Tensor<Int>(2, 2, 2);
tensor.set(0, 0, 0, new Int(1));
tensor.set(0, 0, 1, new Int(2));
tensor.set(0, 1, 0, new Int(3));
tensor.set(0, 1, 1, new Int(4));
tensor.set(1, 0, 0, new Int(5));
tensor.set(1, 0, 1, new Int(6));
tensor.set(1, 1, 0, new Int(7));
tensor.set(1, 1, 1, new Int(8));

print("Layer 0 sum: " + tensor.sumLayer(0));
print("Layer 1 sum: " + tensor.sumLayer(1));

// Test 5: Jagged arrays
testJaggedArrays();

// Test 6: 3D arrays
test3DArrays();

// Test 7: Array processor with collections
print("--- Array processor with collections ---");
int[][] testMatrix = [[1, 2], [3, 4]];
ArrayProcessor processor = new ArrayProcessor();

List<Int> processed = processor.processMatrix(testMatrix);
print("Processed list size: " + processed.size());
for (int i = 0; i < processed.size(); i = i + 1) {
    print("Element " + i + ": " + processed.get(i).getValue());
}

// Test 8: Flatten to 1D
print("--- Flatten to 1D ---");
int[] flattened = processor.flattenTo1D(testMatrix);
string flat = "Flattened: ";
for (int i = 0; i < flattened.length; i = i + 1) {
    flat = flat + flattened[i] + " ";
}
print(flat);

print("=== Test 08 Complete ===");
