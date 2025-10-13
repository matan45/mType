// Error: Duplicate global function name declaration

function calculateSum(int a, int b): int {
    return a + b;
}

// ERROR: Duplicate function with same name
function calculateSum(int x, int y): int {
    return x + y + 10;
}

print("This should not execute");
