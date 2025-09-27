// Test that bool primitive type is rejected in generics
// This should fail with "Invalid type arguments" error

class BooleanWrapper<T> {
    T flag;

    constructor() {
        // Default constructor
    }

    function setFlag(T val): void {
        flag = val;
    }

    function getFlag(): T {
        return flag;
    }
}

function main(): void {
    // This should fail - bool is a primitive type
    BooleanWrapper<bool> boolWrapper = new BooleanWrapper<bool>();
    boolWrapper.setFlag(true);
    print(boolWrapper.getFlag());
}

main();