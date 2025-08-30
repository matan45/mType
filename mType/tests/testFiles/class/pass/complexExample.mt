class Base {
    int x;
    float y;
    string name;
    
    // Static data member
    static final string TYPE = "BaseType";
    static int counter = 0;
    
    // Constructor with parameters
    constructor(int _x, string _name) {
        x = _x;
        name = _name;
        counter = counter + 1;
    }
    
    // Default constructor
    constructor() {
        x = 0;
        name = "";
        counter = counter + 1;
    }
    
    // Static method
    static function getType(): string {
        return TYPE;
    }
    
    // Instance method
    function display(): void {
        print(x);
    }
    
}

class SomeClass {
    Base obj;
    
    // Static member
    static int instances = 0;
    
    // Constructor with parameter
    constructor(Base _obj) {
        obj = _obj;
        instances = instances + 1;
    }
    
    // Default constructor
    constructor() {
        obj = new Base();
        instances = instances + 1;
    }
    
    // Static method returning Base
    static function createSample(): Base {
        return new Base(5, "sample");
    }
    
    // Getter method
    function getObj(): Base {
        return obj;
    }
}

// Create Base object
Base base1 = new Base(10, "First");

// Access static member
print(Base::counter);  // 1

// Access instance data member
print(base1.x);  // 10
base1.display();  // 10


// Assign Base to null
Base base2 = base1;
base1 = null;  // Now base1 is null, but base2 still points to the object
base2.display();  // 10

// Create SomeClass instance
SomeClass sc1 = new SomeClass(base2);

// Static call to create new Base
Base newBase = SomeClass::createSample();
newBase.display();  // 5

// Access static variable of SomeClass
print(SomeClass::instances);  // 1