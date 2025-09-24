// Multi-dimensional array bounds test
function testMultiDimensionalBounds(): void {
    print("Testing multi-dimensional bounds");

    int[][][] array3D = new int[10][20][30];

    // This should cause an out of bounds error
    array3D[10][15][25] = 42;
}

testMultiDimensionalBounds();