class Base {
    public int x;
    public float y;
    public string name;

    // Static data member
    public static final string TYPE = "BaseType";
    public static int counter = 0;

    // Constructor with parameters
    public constructor(int _x, string _name) {
        x = _x;
        name = _name;
        counter = counter + 1;
    }

    // Default constructor
    public constructor() {
        x = 0;
        name = "";
        counter = counter + 1;
    }

    // Static method
    public static function getType(): string {
        return TYPE;
    }

    // Instance method
    public function display(): void {
        print(x);
    }

}

class SomeClass {
    public Base obj;

    // Static member
    public static int instances = 0;

    // Constructor with parameter
    public constructor(Base _obj) {
        obj = _obj;
        instances = instances + 1;
    }

    // Default constructor
    public constructor() {
        obj = new Base();
        instances = instances + 1;
    }

    // Static method returning Base
    public static function createSample(): Base {
        return new Base(5, "sample");
    }

    // Getter method
    public function getObj(): Base {
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