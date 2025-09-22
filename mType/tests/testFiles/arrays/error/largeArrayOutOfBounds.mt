// Large array out of bounds test
function testLargeArrayBounds(): void {
    print("Testing large array bounds");

    int[][] largeArray = new int[200][200];

    // This should cause an out of bounds error
    largeArray[200][200] = 42;
}

testLargeArrayBounds();