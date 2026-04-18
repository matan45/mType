// TARGET: ~2s on dev machine. Adjust ITERS if first run lands outside 1-5s.
// Exercises ARRAY_GET on FlatMultiArray (multi-dim indexing) inside a hot
// function so the JIT function-level compiler / OSR picks it up.
//
// The widened jit_array_get helper now handles FlatMultiArray and
// SparseMultiArray sub-array views in addition to NativeArray. This
// benchmark validates that the multi-dim path produces the same value as
// the interpreter — a wrong dispatch here would corrupt the running sum.
//
// Coverage for MYT-152 (ARRAY_GET ungating).

function sumGrid(int[][] grid, int rows, int cols): int {
    int s = 0;
    for (int i = 0; i < rows; i = i + 1) {
        for (int j = 0; j < cols; j = j + 1) {
            s = s + grid[i][j];
        }
    }
    return s;
}

int ROWS = 32;
int COLS = 32;
int[][] grid = new int[ROWS][COLS];
for (int i = 0; i < ROWS; i = i + 1) {
    for (int j = 0; j < COLS; j = j + 1) {
        grid[i][j] = i * COLS + j;
    }
}

int ITERS = 5000;
int total = 0;
for (int k = 0; k < ITERS; k = k + 1) {
    total = total + sumGrid(grid, ROWS, COLS);
}

print("array_multi_get total=" + total);
