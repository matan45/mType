// Error: Wrong number of type arguments
class Pair<T, U> {
    T first;
    U second;
}

function main(): void {
    // Should fail: Pair expects 2 type arguments but only 1 provided
    Pair<int> pair = new Pair<int>();
}

main();