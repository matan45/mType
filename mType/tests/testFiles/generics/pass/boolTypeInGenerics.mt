// Test that Bool type is now allowed in generics (Pure OOP)
// This should PASS - Bool is now a proper object type

class Wrapper<T> {
    T wrapped;

    constructor(T val) {
        wrapped = val;
    }

    function unwrap(): T {
        return wrapped;
    }
}

function main(): void {
    // Bool is now an object type and can be used in generics!
    Wrapper<Bool> boolWrapper = new Wrapper<Bool>(new Bool(true));
    Bool result = boolWrapper.unwrap();
    print(result.toString());
}

main();
