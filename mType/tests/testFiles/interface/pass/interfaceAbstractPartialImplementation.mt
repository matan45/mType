// Test abstract class implements partial interface
// @Script

interface Drawable {
    function draw(): void;
    function getColor(): string;
    function getSize(): int;
}

abstract class Shape implements Drawable {
    private int size;

    public constructor(int size) {
        this.size = size;
    }

    // Implement some interface methods
    public function getSize(): int {
        return this.size;
    }

    // Leave draw() and getColor() for subclasses - declare as abstract
    public abstract function draw(): void;
    public abstract function getColor(): string;
}

class Circle extends Shape {
    private string color;

    public constructor(int size, string color) : super(size) {
        this.color = color;
    }

    public function draw(): void {
        print("Drawing circle");
    }

    public function getColor(): string {
        return this.color;
    }
}

class Rectangle extends Shape {
    private string color;

    public constructor(int size, string color) : super(size) {
        this.color = color;
    }

    public function draw(): void {
        print("Drawing rectangle");
    }

    public function getColor(): string {
        return this.color;
    }
}

Circle circle = new Circle(10, "red");
circle.draw();
print(circle.getColor());
print(circle.getSize());

Rectangle rect = new Rectangle(20, "blue");
rect.draw();
print(rect.getColor());
print(rect.getSize());
