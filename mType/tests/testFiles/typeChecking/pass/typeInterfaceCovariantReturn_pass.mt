// Test covariant return types in interface implementation
class Shape {
    string name;

    constructor(string n) {
        this.name = n;
    }

    public function getName(): string {
        return this.name;
    }
}

class Circle extends Shape {
    float radius;

    constructor(float r): super("Circle") {
        this.radius = r;
    }

    public function getRadius(): float {
        return this.radius;
    }
}

interface ShapeFactory {
    public function create(): Shape;
}


// Covariant return: Circle is a subtype of Shape
class ConcreteFactory implements ShapeFactory {
    public function create(): Shape {
        return new Circle(5.0);
    }
}

ConcreteFactory factory = new ConcreteFactory();
Shape shape = factory.create();
Circle circle = (Circle)factory.create();

print(shape.getName());
print(circle.getName());
print(circle.getRadius());
print("Covariant return types successful");
