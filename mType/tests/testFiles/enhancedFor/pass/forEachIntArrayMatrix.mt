// Edge: nested for-each over int[][] with both levels using the array-fast
// path. The synthetic __foreach_array__/length/counter locals must be unique
// per nested level — slot collisions would corrupt the inner loop's counter.
function main(): void {
    int[][] grid = [[1, 2, 3], [4, 5, 6]];
    for (int[] row : grid) {
        for (int v : row) {
            print(v);
        }
        print("--");
    }
}

main();
