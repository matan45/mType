// ERROR: Jagged array dimension mismatch

function main(): void {
    // Create jagged array
    int[][] jagged = new int[3][];
    jagged[0] = new int[2];
    jagged[1] = new int[3];
    jagged[2] = new int[1];

    // ERROR: Cannot assign rectangular array to jagged variable
    int[2][3] rectangular = new int[2][3];
    int[][] jaggedRef = rectangular;  // Type mismatch

    print("This should not execute");
}

main();
