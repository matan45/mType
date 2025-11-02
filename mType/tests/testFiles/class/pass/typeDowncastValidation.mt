// Test: Downcast with runtime type checking
// Expected: Pass - demonstrates safe downcasting

class Shape {
    public void draw() {
        print("Drawing shape");
    }
}

class Circle extends Shape {
    private int radius;

    public constructor(int radius) {
        this.radius = radius;
    }

    public void draw() {
        print("Drawing circle with radius " + this.radius);
    }

    public int getRadius() {
        return this.radius;
    }
}

class Square extends Shape {
    private int side;

    public constructor(int side) {
        this.side = side;
    }

    public void draw() {
        print("Drawing square with side " + this.side);
    }

    public int getSide() {
        return this.side;
    }
}

// Test downcasting
print("Test 1: Valid downcast - Circle");
Shape s1 = new Circle(5);
s1.draw();
if (s1 instanceof Circle) {
    Circle c = (Circle)s1;
    print("Downcast successful, radius: " + c.getRadius());
}

print("\nTest 2: Valid downcast - Square");
Shape s2 = new Square(10);
s2.draw();
if (s2 instanceof Square) {
    Square sq = (Square)s2;
    print("Downcast successful, side: " + sq.getSide());
}

print("\nTest 3: Type checking before downcast");
Shape s3 = new Circle(7);
if (s3 instanceof Circle) {
    print("s3 is a Circle");
} else if (s3 instanceof Square) {
    print("s3 is a Square");
} else {
    print("s3 is a Shape");
}
