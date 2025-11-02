// Test polymorphic array element access
print("Testing polymorphic array access");

class Shape {
    string name;

    constructor(string n) {
        name = n;
    }

    public function describe(): string {
        return "Shape: " + name;
    }

    public function area(): int {
        return 0;
    }
}

class Square : Shape {
    int side;

    constructor(int s) : Shape("Square") {
        side = s;
    }

    public function area(): int {
        return side * side;
    }

    public function describe(): string {
        return "Square with side " + side + ", area " + area();
    }
}

class Triangle : Shape {
    int base;
    int height;

    constructor(int b, int h) : Shape("Triangle") {
        base = b;
        height = h;
    }

    public function area(): int {
        return (base * height) / 2;
    }

    public function describe(): string {
        return "Triangle with area " + area();
    }
}

Shape[] shapes = new Shape[3];
shapes[0] = new Square(5);
shapes[1] = new Triangle(10, 6);
shapes[2] = new Square(3);

print("Polymorphic access:");
for (int i = 0; i < shapes.length; i++) {
    print("  " + shapes[i].describe());
}

print("Polymorphic array access test completed");
