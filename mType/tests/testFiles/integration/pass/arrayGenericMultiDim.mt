// Arrays + Generics Test 6: Multi-dimensional generic arrays
@Script

class Matrix<T> {
    field data: T[][];
    field rows: Int;
    field cols: Int;

    constructor(r: Int, c: Int) {
        this.rows = r;
        this.cols = c;
        this.data = T[][r];
        let i: Int = 0;
        while (i < r) {
            this.data[i] = T[c];
            i = i + 1;
        }
    }

    fun set(row: Int, col: Int, value: T): Void {
        this.data[row][col] = value;
    }

    fun get(row: Int, col: Int): T {
        return this.data[row][col];
    }

    fun print(): Void {
        let i: Int = 0;
        while (i < this.rows) {
            let j: Int = 0;
            while (j < this.cols) {
                print(this.data[i][j]);
                j = j + 1;
            }
            i = i + 1;
        }
    }

    fun transpose(): Matrix<T> {
        let result: Matrix<T> = Matrix<T>(this.cols, this.rows);
        let i: Int = 0;
        while (i < this.rows) {
            let j: Int = 0;
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
let intMatrix: Matrix<Int> = Matrix<Int>(3, 2);
intMatrix.set(0, 0, 1);
intMatrix.set(0, 1, 2);
intMatrix.set(1, 0, 3);
intMatrix.set(1, 1, 4);
intMatrix.set(2, 0, 5);
intMatrix.set(2, 1, 6);

print("Original matrix (3x2):");
intMatrix.print();

let transposed: Matrix<Int> = intMatrix.transpose();
print("Transposed matrix (2x3):");
transposed.print();

print("Creating 2x3 string matrix:");
let strMatrix: Matrix<String> = Matrix<String>(2, 3);
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
