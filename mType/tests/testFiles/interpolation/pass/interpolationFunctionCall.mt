// Test: function call inside string interpolation
function compute(int a, int b): int {
    return a * b;
}

print($"result: {compute(3, 4)}");
print($"sum-and-product: {compute(2, 5)} from compute(2,5)");
