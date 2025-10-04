class Rectangle {
    public int width;
    public int height;

    // Default constructor
    public constructor() {
        width = 0;
        height = 0;
    }

    // Constructor with parameters
    public constructor(int w, int h) {
        width = w;
        height = h;
    }

    // Constructor with single parameter (square)
    public constructor(int size) {
        width = size;
        height = size;
    }

    public function area(): int {
        return width * height;
    }
}

Rectangle r1 = new Rectangle();
print(r1.area()); // 0

Rectangle r2 = new Rectangle(5, 3);
print(r2.area()); // 15

Rectangle r3 = new Rectangle(4);
print(r3.area()); // 16