// Error: pins BY-DESIGN view semantics — sub-arrays of a multi-dim array are
// views into the flat backing store, so replacing a whole row
// (arr[i] = newRow) is rejected at runtime. This is intended FlatMultiArray
// behavior, not a bug; element-wise writes are the supported mutation.
function main(): void {
    int[][] grid = new int[3][4];
    grid[1][2] = 42;
    print(grid[1][2]);

    int[] newRow = new int[4];
    newRow[0] = 9;
    grid[1] = newRow;
    print(grid[1][0]);
}
main();
