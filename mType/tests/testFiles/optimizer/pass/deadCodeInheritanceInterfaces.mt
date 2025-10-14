// Test file for O1/O2 optimization - Dead Code Elimination with Inheritance and Interfaces
// Tests that extends and implements are preserved while dead code is removed from methods
import * from "../../lib/primitives/String.mt";
// Base interface
interface Drawable {
    function draw(): void;
}

// Another interface
interface Resizable {
    function resize(int width, int height): void;
}

// Base class
class Shape {
    protected string color;

    public constructor(string color) {
        this.color = color;
    }

    public function getColor(): string {
        return this.color;
        print("Dead after return in base class");  // Should be removed
        string unused = "test";                     // Should be removed
    }

    public function describe(): void {
        print("This is a shape");
        return;
        print("Dead after return in describe");  // Should be removed
    }
}

// Derived class with inheritance and interface implementation
class Rectangle extends Shape implements Drawable, Resizable {
    private int width;
    private int height;

    public constructor(string color, int w, int h) : super(color) {
        this.width = w;
        this.height = h;
    }

    public function draw(): void {
        print("Drawing rectangle: " + this.color);
        return;
        print("Dead after return in draw");  // Should be removed
        this.width = 0;                       // Should be removed
    }

    public function resize(int w, int h): void {
        if (w < 0 || h < 0) {
            throw new Exception("Invalid dimensions");
            print("Dead after throw in resize");  // Should be removed
            return;                                // Should be removed
        }
        this.width = w;
        this.height = h;
    }

    public function getArea(): int {
        int area = this.width * this.height;
        return area;
        print("Dead after return in getArea");  // Should be removed
        area = 0;                                // Should be removed
    }
}

// Another derived class
class Circle extends Shape implements Drawable {
    private int radius;

    public constructor(string color, int r) : super(color) {
        this.radius = r;
    }

    public function draw(): void {
        print("Drawing circle: " + this.color);
        if (this.radius > 100) {
            return;
            print("Dead in if branch");  // Should be removed
        }
        print("Circle drawn");
    }

    public function getRadius(): int {
        return this.radius;
        print("Dead after return");  // Should be removed
        this.radius = 0;              // Should be removed
    }
}

// Generic class with inheritance
class Box<T> extends Shape {
    private T content;

    public constructor(string color, T item) : super(color) {
        this.content = item;
    }

    public function getContent(): T {
        return this.content;
        print("Dead after return in generic method");  // Should be removed
    }

    public function setContent(T item): void {
        this.content = item;
        return;
        print("Dead after return in setter");  // Should be removed
    }
}

// Final class (cannot be extended)
final class ImmutablePoint implements Drawable {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function draw(): void {
        print("Point at (" + this.x + ", " + this.y + ")");
        return;
        print("Dead after return in final class");  // Should be removed
    }

    public function getX(): int {
        if (this.x < 0) {
            return 0;
            print("Dead in negative check");  // Should be removed
        }
        return this.x;
    }
}

// Test deep inheritance chain
class Animal {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public function speak(): string {
        return "Some sound";
        print("Dead in base Animal");  // Should be removed
    }
}

class Mammal extends Animal {
    protected bool hasFur;

    public constructor(string name, bool fur) : super(name) {
        this.hasFur = fur;
    }

    public function speak(): string {
        return "Mammal sound";
        print("Dead in Mammal");  // Should be removed
    }
}

class Dog extends Mammal {
    private string breed;

    public constructor(string name, string breed) : super(name, true) {
        this.breed = breed;
    }

    public function speak(): string {
        return "Woof!";
        print("Dead in Dog");  // Should be removed
    }

    public function getBreed(): string {
        return this.breed;
        print("Dead after return");  // Should be removed
    }
}

// Multiple interface implementation with dead code
interface Printable {
    function print(): void;
}

interface Serializable {
    function serialize(): string;
}

class Document implements Printable, Serializable {
    private string content;

    public constructor(string content) {
        this.content = content;
    }

    public function print(): void {
        print("Printing: " + this.content);
        return;
        print("Dead after return in print");  // Should be removed
    }

    public function serialize(): string {
        string result = "{content: \"" + this.content + "\"}";
        return result;
        print("Dead after return in serialize");  // Should be removed
    }
}

// Entry point
print("=== Testing Dead Code Elimination with Inheritance and Interfaces ===");

// Test Rectangle (extends Shape, implements Drawable + Resizable)
Rectangle rect = new Rectangle("blue", 10, 20);
print("Rectangle color: " + rect.getColor());
rect.draw();
print("Rectangle area: " + rect.getArea());
rect.resize(15, 25);

// Test Circle (extends Shape, implements Drawable)
Circle circle = new Circle("red", 50);
circle.draw();
print("Circle radius: " + circle.getRadius());

// Test generic Box (extends Shape)
Box<String> box = new Box<String>("green", new String("treasure"));
print("Box content: " + box.getContent());
box.setContent("gold");

// Test final ImmutablePoint (implements Drawable)
ImmutablePoint point = new ImmutablePoint(10, 20);
point.draw();
print("Point X: " + point.getX());

// Test deep inheritance
Dog dog = new Dog("Buddy", "Golden Retriever");
print("Dog says: " + dog.speak());
print("Dog breed: " + dog.getBreed());

// Test multiple interfaces
Document doc = new Document("Hello World");
doc.print();
print("Serialized: " + doc.serialize());

print("Inheritance and Interfaces Dead Code Test Complete!");
