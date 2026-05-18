// Test enhanced for-loop over primitive float[] array
function main(): void {
    print("Testing enhanced for-loop over float[]:");

    float[] prices = [1.5, 2.5, 3.5, 4.5];
    int count = 0;
    for (float p : prices) {
        print(p);
        count = count + 1;
    }
    print("Count: " + count);

    // Sized-then-assigned float[]
    float[] grades = new float[3];
    grades[0] = 9.0;
    grades[1] = 7.5;
    grades[2] = 8.25;
    for (float g : grades) {
        print(g);
    }

    print("for-each over float[] test passed!");
}

main();
