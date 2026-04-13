// MathLib: Shape hierarchy for cross-library inheritance testing

abstract class Shape {
    public string name;

    public constructor(string name) {
        this.name = name;
    }

    abstract function area(): float;

    public function describe(): string {
        return this.name + " with area " + this.area();
    }
}

class Circle extends Shape {
    public float radius;

    public constructor(float radius) : super("Circle") {
        this.radius = radius;
    }

    public function area(): float {
        return 3.14 * this.radius * this.radius;
    }
}

class Rectangle extends Shape {
    public float width;
    public float height;

    public constructor(float w, float h) : super("Rectangle") {
        this.width = w;
        this.height = h;
    }

    public function area(): float {
        return this.width * this.height;
    }
}
