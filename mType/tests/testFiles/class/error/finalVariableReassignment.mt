final class Base {
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
}

final Base base3 = new Base(10, "First");
base3 = null; // Error: Cannot assign to final variable