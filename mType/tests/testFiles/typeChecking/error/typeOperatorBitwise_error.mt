// ERROR: Bitwise operators on non-integral types

@Script
function main(): void {
    float a = 5.5;
    float b = 3.2;

    // ERROR: Cannot apply bitwise AND operator to float types
    int result = a & b;

    print("This should not execute");
}

main();
