// ERROR: Test negative or invalid array sizes

function testNegativeSize(): void {
    // ERROR: Negative array size
    int[] arr = new int[-5];
    print("This should not execute");
}

function testZeroSize(): void {
    // Zero size is valid - this is just to show the boundary
    int[] arr = new int[0];
    print("Zero size array created");
}

function testNegativeMultiDim(): void {
    // ERROR: Negative dimension in multidimensional array
    int[][] matrix = new int[3][-2];
    print("This should not execute");
}

// Call the error-inducing function
testNegativeSize();
