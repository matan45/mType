// Arrays + Generics Test 6: Multi-dimensional generic arrays
@Script

class Matrix<T> {
    T[][] data;
    Int rows;
    Int cols;

    constructor(Int r, Int c) {
        this.rows = r;
        this.cols = c;
        this.data = T[][r];
        Int i = 0;
        while (i < r) {
            this.data[i] = T[c];
            i = i + 1;
        }
    }

    public function set(Int row, Int col, T value): void {
        this.data[row][col] = value;
    }

    public function get(Int row, Int col): T {
        return this.data[row][col];
    }

    public function print(): void {
        Int i = 0;
        while (i < this.rows) {
            Int j = 0;
            while (j < this.cols) {
                print(this.data[i][j]);
                j = j + 1;
            }
            i = i + 1;
        }
    }

    public function transpose(): Matrix<T> {
        Matrix<T> result = Matrix<T>(this.cols, this.rows);
        Int i = 0;
        while (i < this.rows) {
            Int j = 0;
            while (j < this.cols) {
                result.set(j, i, this.get(i, j));
                j = j + 1;
            }
            i = i + 1;
        }
        return result;
    }
}

print("Creating 3x2 integer matrix:");
Matrix<Int> intMatrix = Matrix<Int>(3, 2);
intMatrix.set(0, 0, 1);
intMatrix.set(0, 1, 2);
intMatrix.set(1, 0, 3);
intMatrix.set(1, 1, 4);
intMatrix.set(2, 0, 5);
intMatrix.set(2, 1, 6);

print("Original matrix (3x2):");
intMatrix.print();

Matrix<Int> transposed = intMatrix.transpose();
print("Transposed matrix (2x3):");
transposed.print();

print("Creating 2x3 string matrix:");
Matrix<String> strMatrix = Matrix<String>(2, 3);
strMatrix.set(0, 0, "a");
strMatrix.set(0, 1, "b");
strMatrix.set(0, 2, "c");
strMatrix.set(1, 0, "d");
strMatrix.set(1, 1, "e");
strMatrix.set(1, 2, "f");

print("String matrix:");
strMatrix.print();

print("Accessing element (1, 2):");
print(strMatrix.get(1, 2));
