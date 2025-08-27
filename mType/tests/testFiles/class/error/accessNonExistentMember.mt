class SimpleClass {
    int existingField;
    
    constructor() {
        existingField = 42;
    }
    
    function existingMethod(): int {
        return existingField;
    }
}

SimpleClass obj = new SimpleClass();
print(obj.existingField); // 42 - This should work
print(obj.existingMethod()); // 42 - This should work

// These should error
print(obj.nonExistentField); // Error: Unknown member
print(obj.nonExistentMethod()); // Error: Unknown method