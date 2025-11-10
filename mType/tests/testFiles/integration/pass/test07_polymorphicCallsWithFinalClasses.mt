// Integration Test 07: Polymorphic Calls with Final Classes
// Tests: Inheritance + Polymorphism + Final classes/methods + isClassOf + Static members

// Base class with final and non-final methods
class Shape {
    protected string name;
    protected static int shapeCount = 0;

    constructor(string n) {
        this.name = n;
        Shape::shapeCount = Shape::shapeCount + 1;
    }

    // Final method - cannot be overridden
    public final function getName(): string {
        return this.name;
    }

    // Virtual method - can be overridden
    public function getArea(): float {
        return 0.0;
    }

    // Virtual method - can be overridden
    public function describe(): string {
        return "Shape: " + this.name;
    }

    public static function getCount(): int {
        return Shape::shapeCount;
    }
}

// Derived class overriding non-final methods
class Circle extends Shape {
    private float radius;

    constructor(string n, float r) : super(n) {
        this.radius = r;
    }

    // Override getArea
    @Override
    public function getArea(): float {
        return 3.14159 * this.radius * this.radius;
    }

    // Override describe
    @Override
    public function describe(): string {
        return "Circle: " + this.name + ", radius=" + this.radius;
    }

    public function getRadius(): float {
        return this.radius;
    }
}

// Another derived class
class Rectangle extends Shape {
    protected float width;
    private float height;

    constructor(string n, float w, float h) : super(n) {
        this.width = w;
        this.height = h;
    }

    @Override
    public function getArea(): float {
        return this.width * this.height;
    }

    @Override
    public function describe(): string {
        return "Rectangle: " + this.name + ", " + this.width + "x" + this.height;
    }
}

// Final class - cannot be extended
final class Square extends Rectangle {
    constructor(string n, float size) : super(n, size, size) {}

    // Can override parent methods in final class
    @Override
    public function describe(): string {
        return "Square: " + this.getName() + ", size=" + this.width;
    }
}

// Class with final methods
class Animal {
    protected string species;
    protected static int animalCount = 0;

    constructor(string s) {
        this.species = s;
        Animal::animalCount = Animal::animalCount + 1;
    }

    // Final method
    public final function getSpecies(): string {
        return this.species;
    }

    // Non-final method
    public function makeSound(): string {
        return "Generic animal sound";
    }

    public static function getTotalAnimals(): int {
        return Animal::animalCount;
    }
}

class Dog extends Animal {
    private string breed;

    constructor(string b) : super("Canine") {
        this.breed = b;
    }

    // Can override non-final method
    @Override
    public function makeSound(): string {
        return "Woof! I am a " + this.breed;
    }

    public function getBreed(): string {
        return this.breed;
    }
}

// Test polymorphism with isClassOf
function processShape(Shape shape): void {
    print("Processing: " + shape.describe());
    print("Area: " + shape.getArea());
    print("Name (final method): " + shape.getName());

    // Type checking with isClassOf
    if (shape isClassOf Circle) {
        print("Type: Circle detected");
        Circle c = (Circle)shape;
        print("Radius: " + c.getRadius());
    } else if (shape isClassOf Square) {
        print("Type: Square detected");
    } else if (shape isClassOf Rectangle) {
        print("Type: Rectangle detected");
    } else {
        print("Type: Generic Shape");
    }
}

// Test polymorphism with animals
function testAnimal(Animal animal): void {
    print("Species (final method): " + animal.getSpecies());
    print("Sound: " + animal.makeSound());

    if (animal isClassOf Dog) {
        print("This is a Dog");
        Dog d = (Dog)animal;
        print("Breed: " + d.getBreed());
    }
}

// Main test execution
print("=== Test 07: Polymorphic Calls with Final Classes ===");

// Test 1: Shape polymorphism
print("--- Shape polymorphism ---");
print("Initial shape count: " + Shape::getCount());

Circle circle = new Circle("MyCircle", 5.0);
Rectangle rect = new Rectangle("MyRect", 4.0, 6.0);
Square square = new Square("MySquare", 3.0);

print("Total shapes created: " + Shape::getCount());

// Test polymorphic calls through base reference
Shape shape1 = circle;
processShape(shape1);

print("---");
Shape shape2 = rect;
processShape(shape2);

print("---");
Shape shape3 = square;
processShape(shape3);

// Test 2: Final method access
print("--- Final method access ---");
print("Circle name: " + circle.getName());
print("Rectangle name: " + rect.getName());
print("Square name: " + square.getName());

// Test 3: Animal polymorphism
print("--- Animal polymorphism ---");
print("Initial animal count: " + Animal::getTotalAnimals());

Dog dog1 = new Dog("Labrador");
Dog dog2 = new Dog("Bulldog");

print("Total animals: " + Animal::getTotalAnimals());

Animal animalRef1 = dog1;
testAnimal(animalRef1);

print("---");
Animal animalRef2 = dog2;
testAnimal(animalRef2);

// Test 4: Static method calls
print("--- Static method calls ---");
print("Shape count via static method: " + Shape::getCount());
print("Animal count via static method: " + Animal::getTotalAnimals());

// Test 5: Type hierarchy checking
print("--- Type hierarchy with isClassOf ---");
print("square isClassOf Square: " + (square isClassOf Square));
print("square isClassOf Rectangle: " + (square isClassOf Rectangle));
print("square isClassOf Shape: " + (square isClassOf Shape));
print("circle isClassOf Square: " + (circle isClassOf Square));

print("dog1 isClassOf Dog: " + (dog1 isClassOf Dog));
print("dog1 isClassOf Animal: " + (dog1 isClassOf Animal));

print("=== Test 07 Complete ===");
