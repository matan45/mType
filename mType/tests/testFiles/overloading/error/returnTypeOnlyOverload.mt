// Methods differing only by return type - rejected. Return type is NOT part of the
// overload signature, so the registrar sees identical signatures and throws.
class Formatter {
    int describe(int x) {
        return x;
    }

    // ERROR: Duplicate method signature: 'describe(int)'  (return type ignored for overload key)
    string describe(int x) {
        return "value: " + x;
    }
}

@Script
function main() {
    Formatter f = new Formatter();
    print(f.describe(7));
}
