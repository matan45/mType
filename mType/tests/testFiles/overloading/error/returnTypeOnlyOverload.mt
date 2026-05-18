// Methods differing only by return type - rejected. Return type is NOT part of the
// overload signature, so the registrar sees identical signatures and throws.
class Formatter {
    public function describe(int x): int {
        return x;
    }

    // ERROR: Duplicate method signature: 'describe(int)'  (return type ignored for overload key)
    public function describe(int x): string {
        return "value: " + x;
    }
}

function main(): void {
    Formatter f = new Formatter();
    print(f.describe(7));
}
