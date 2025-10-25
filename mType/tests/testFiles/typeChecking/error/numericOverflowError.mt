// Test numeric overflow detection

function main(): void {
    int maxInt = 2147483647;  // Max 32-bit int

    // Overflow: should detect or wrap
    int overflow = maxInt + 1;

    print("overflow value: " + overflow);

    // Large multiplication
    int big1 = 100000000000000000000000000000000000;
    int big2 = 100000000000000000000000000000000000;
    int product = big1 * big2;  // Overflow

    print("product: " + product);

    print("Overflow test completed");
}

main();
