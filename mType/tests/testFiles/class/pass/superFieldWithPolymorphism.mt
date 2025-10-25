// Test super field access combined with polymorphism

class Shape {
    protected int x;
    protected int y;
    protected string color;

    constructor(int posX, int posY) {
        x = posX;
        y = posY;
        color = "black";
    }

    public function getPosition(): string {
        return "(" + x + ", " + y + ")";
    }

    public function describe(): string {
        return "Shape at " + getPosition() + " with color " + color;
    }
}

class Circle extends Shape {
    private int radius;

    constructor(int posX, int posY, int r): super(posX, posY) {
        radius = r;
    }

    // Override and use super fields
    public function describe(): string {
        return "Circle at " + super.getPosition() + " with radius " + radius + ", color: " + super.color;
    }

    public function move(int deltaX, int deltaY): void {
        super.x = super.x + deltaX;
        super.y = super.y + deltaY;
    }

    public function setColor(string newColor): void {
        super.color = newColor;
    }
}

class Rectangle extends Shape {
    private int width;
    private int height;

    constructor(int posX, int posY, int w, int h): super(posX, posY) {
        width = w;
        height = h;
    }

    // Override and use super fields
    public function describe(): string {
        return "Rectangle at " + super.getPosition() + " with size " + width + "x" + height + ", color: " + super.color;
    }

    public function move(int deltaX, int deltaY): void {
        super.x = super.x + deltaX;
        super.y = super.y + deltaY;
    }

    public function setColor(string newColor): void {
        super.color = newColor;
    }
}

function printShapeInfo(Shape shape): void {
    print(shape.describe());
}

function main(): void {
    print("Testing super fields with polymorphism");

    Circle circle = new Circle(10, 20, 5);
    Rectangle rect = new Rectangle(30, 40, 15, 25);

    // Test polymorphic calls
    printShapeInfo(circle);
    printShapeInfo(rect);

    // Modify super fields
    circle.setColor("red");
    circle.move(5, 10);
    rect.setColor("blue");
    rect.move(-5, -10);

    print("After modifications:");
    printShapeInfo(circle);
    printShapeInfo(rect);

    print("Polymorphism with super fields test completed");
}

main();
