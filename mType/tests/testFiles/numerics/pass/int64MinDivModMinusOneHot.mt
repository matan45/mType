// Test: INT64_MIN / -1 and % -1 stay defined after hot-loop specialization.
function divEdge(int value): int {
    return value / -1;
}

function modEdge(int value): int {
    return value % -1;
}

function boxedDivEdge(Int value, Int divisor): int {
    return value.divide(divisor).getValue();
}

function boxedModEdge(Int value, Int divisor): int {
    return value.modulo(divisor).getValue();
}

function main(): void {
    int min = 1 << 63;
    int divResult = 0;
    int modResult = -1;
    int boxedDivResult = 0;
    int boxedModResult = -1;
    Int boxedMin = new Int(min);
    Int minusOne = new Int(-1);

    for (int i = 0; i < 300; i = i + 1) {
        divResult = divEdge(min);
        modResult = modEdge(min);
        boxedDivResult = boxedDivEdge(boxedMin, minusOne);
        boxedModResult = boxedModEdge(boxedMin, minusOne);
    }

    print(divResult);
    print(modResult);
    print(boxedDivResult);
    print(boxedModResult);
}
main();

// Expected output:
// -9223372036854775808
// 0
// -9223372036854775808
// 0
