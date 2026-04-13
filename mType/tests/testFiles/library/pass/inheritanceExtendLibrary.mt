class Shape {
    public string name;

    public constructor(string name) {
        this.name = name;
    }

    public function area(): float {
        return 0.0;
    }

    public function describe(): string {
        return this.name;
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

Shape s = new Circle(5.0);
print(s.describe());
print(s.area());
