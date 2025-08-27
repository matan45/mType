class ComplexClass {
    // Instance members
    int instanceInt;
    float instanceFloat;
    string instanceString;
    
    // Static members
    static int staticCounter = 0;
    static final string CLASS_NAME = "ComplexClass";
    
    // Default constructor
    constructor() {
        instanceInt = 0;
        instanceFloat = 0.0;
        instanceString = "default";
        staticCounter = staticCounter + 1;
    }
    
    // Parameterized constructor
    constructor(int i, float f, string s) {
        instanceInt = i;
        instanceFloat = f;
        instanceString = s;
        staticCounter = staticCounter + 1;
    }
    
    // Instance methods
    function setValues(int i, float f, string s): void {
        instanceInt = i;
        instanceFloat = f;
        instanceString = s;
    }
    
    function display(): void {
        print(instanceInt);
        print(instanceFloat);
        print(instanceString);
    }
    
    function calculate(): float {
        return instanceInt * instanceFloat;
    }
    
    // Static methods
    static function getCounter(): int {
        return staticCounter;
    }
    
    static function getClassName(): string {
        return CLASS_NAME;
    }
    
    static function resetCounter(): void {
        staticCounter = 0;
    }
    
}

// Test static access
print(ComplexClass::getClassName()); // ComplexClass
print(ComplexClass::getCounter()); // 0

// Create instances
ComplexClass obj1 = new ComplexClass();
print(ComplexClass::getCounter()); // 1

ComplexClass obj2 = new ComplexClass(42, 3.14, "test");
print(ComplexClass::getCounter()); // 2

// Test instance methods
obj1.setValues(10, 2.5, "modified");
obj1.display(); // 10, 2.5, modified
print(obj1.calculate()); // 25.0

obj2.display(); // 42, 3.14, test
print(obj2.calculate()); // 131.88


// Reset static counter
ComplexClass::resetCounter();
print(ComplexClass::getCounter()); // 0