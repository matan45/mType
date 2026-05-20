// CANARY (MYT-350): nested for-each over int[][] loses inner element type.
// Outer for-each binds `row` as int[] (its declared loop-variable type), but
// the inner for-each then types `v` as Object and rejects with MT-E2007. See
// memory:project_foreach_loses_nested_generic_type. Keep failing until fixed.

function main(): void {
    int[][] matrix = [[1, 2, 3], [4, 5, 6]];

    int sum = 0;
    for (int[] row : matrix) {
        for (int v : row) {
            sum = sum + v;
        }
    }
    print("sum=" + sum);
}

main();

// Expected output (when MYT-350 is fixed):
// sum=21
