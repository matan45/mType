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


// This line should be removed to prevent error:
final Base base3 = new Base(10, "First");
base3 = null; // Error: Cannot assign to final variable