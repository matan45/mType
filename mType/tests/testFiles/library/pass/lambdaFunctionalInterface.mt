interface Transform {
    function apply(int value): int;
}

function applyTransform(int x, Transform t): int {
    return t.apply(x);
}

int result = applyTransform(5, x -> x * 2);
print(result);

int result2 = applyTransform(10, x -> x + 3);
print(result2);
