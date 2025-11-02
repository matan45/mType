// Test polymorphic array element access
print("Testing polymorphic array access");

class Shape {
    string name;

    Shape(string n) {
        name = n;
    }

    string describe() {
        return "Shape: " + name;
    }

    int area() {
        return 0;
    }
}

class Square : Shape {
    int side;

    Square(int s) : Shape("Square") {
        side = s;
    }

    int area() {
        return side * side;
    }

    string describe() {
        return "Square with side " + side + ", area " + area();
    }
}

class Triangle : Shape {
    int base;
    int height;

    Triangle(int b, int h) : Shape("Triangle") {
        base = b;
        height = h;
    }

    int area() {
        return (base * height) / 2;
    }

    string describe() {
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
