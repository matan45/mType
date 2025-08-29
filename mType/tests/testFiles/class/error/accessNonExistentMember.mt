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
print(obj.nonExistentField);