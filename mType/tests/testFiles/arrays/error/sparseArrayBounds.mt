// Sparse array bounds test
function testSparseArrayBounds(): void {
    print("Testing sparse array bounds");

    int[][] sparseArray = new int[150][150]; // Sparse storage

    // This should cause an out of bounds error
    sparseArray[150][149] = 42;
}

testSparseArrayBounds();