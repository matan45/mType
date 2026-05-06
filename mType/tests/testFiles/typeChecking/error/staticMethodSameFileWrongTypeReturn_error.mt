// MYT-279: same-file Class::staticMethod() must be type-checked. The
// caller appears BEFORE the class declaration so the bug repro condition
// (method not yet registered when the caller body is type-checked) is
// exercised. Without the fix, this file silently compiles and runs.
function bad(): int[] {
    return A::names();
}

class A {
    public static function names(): string[] {
        string[] arr = new string[1];
        arr[0] = "x";
        return arr;
    }
}

int[] result = bad();
print(result[0]);
