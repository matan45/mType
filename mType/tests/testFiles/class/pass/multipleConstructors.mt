class Rectangle {
    int width;
    int height;
    
    // Default constructor
    constructor() {
        width = 0;
        height = 0;
    }
    
    // Constructor with parameters
    constructor(int w, int h) {
        width = w;
        height = h;
    }
    
    // Constructor with single parameter (square)
    constructor(int size) {
        width = size;
        height = size;
    }
    
    function area(): int {
        return width * height;
    }
}

Rectangle r1 = new Rectangle();
print(r1.area()); // 0

Rectangle r2 = new Rectangle(5, 3);
print(r2.area()); // 15

Rectangle r3 = new Rectangle(4);
print(r3.area()); // 16