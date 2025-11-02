// Integration Test: Casting in if/else branches and ternary expressions
// Tests type narrowing and casting across different conditional constructs

class Shape {
    public int dimensions;

    public constructor(int d) {
        this.dimensions = d;
    }

    public function describe(): string {
        return "Shape";
    }
}

class Circle extends Shape {
    public float radius;

    public constructor(float r) {
        super(2);
        this.radius = r;
    }

    public function describe(): string {
        return "Circle";
    }

    public function getArea(): float {
        return 3.14 * this.radius * this.radius;
    }
}

class Rectangle extends Shape {
    public float width;
    public float height;

    public constructor(float w, float h) {
        super(2);
        this.width = w;
        this.height = h;
    }

    public function describe(): string {
        return "Rectangle";
    }

    public function getArea(): float {
        return this.width * this.height;
    }

    public function getPerimeter(): float {
        return 2.0 * (this.width + this.height);
    }
}

class Sphere extends Shape {
    public float radius;

    public constructor(float r) {
        super(3);
        this.radius = r;
    }

    public function describe(): string {
        return "Sphere";
    }

    public function getVolume(): float {
        return 4.18 * this.radius * this.radius * this.radius;
    }
}

// Test 1: Simple if/else with casting
print("Test 1: Simple if/else with casting");
Shape s1 = new Circle(5.0);
if (s1 isClassOf Circle) {
    print("Circle area: " + ((Circle)s1).getArea());
} else {
    print("Not a circle");
}

// Test 2: Multiple branches with different casts
print("Test 2: Multiple if/else branches");
Shape shapes[] = new Shape[3];
shapes[0] = new Circle(3.0);
shapes[1] = new Rectangle(4.0, 5.0);
shapes[2] = new Sphere(2.0);

for (int i = 0; i < 3; i = i + 1) {
    if (shapes[i] isClassOf Circle) {
        Circle c = (Circle)shapes[i];
        print("Circle - area: " + c.getArea());
    } else if (shapes[i] isClassOf Rectangle) {
        Rectangle r = (Rectangle)shapes[i];
        print("Rectangle - area: " + r.getArea() + ", perimeter: " + r.getPerimeter());
    } else if (shapes[i] isClassOf Sphere) {
        Sphere sp = (Sphere)shapes[i];
        print("Sphere - volume: " + sp.getVolume());
    } else {
        print("Unknown shape");
    }
}

// Test 3: Nested conditionals with casting
print("Test 3: Nested conditionals with casting");
Shape testShape = new Rectangle(10.0, 20.0);
if (testShape isClassOf Shape) {
    print("Is a shape with " + testShape.dimensions + " dimensions");
    if (testShape.dimensions == 2) {
        print("2D shape detected");
        if (testShape isClassOf Rectangle) {
            Rectangle rect = (Rectangle)testShape;
            if (rect.width > 5.0) {
                print("Wide rectangle: width = " + rect.width);
            }
        }
    }
}

// Test 4: Casting in conditional expressions
print("Test 4: Casting in conditional expressions");
Shape s2 = new Circle(7.0);
if (s2 isClassOf Circle && ((Circle)s2).getArea() > 100.0) {
    print("Large circle");
} else if (s2 isClassOf Circle && ((Circle)s2).getArea() <= 100.0) {
    print("Small circle");
}

// Test 5: Multiple casts in compound condition
print("Test 5: Multiple casts in compound condition");
Shape shape1 = new Circle(5.0);
Shape shape2 = new Rectangle(10.0, 2.0);

if ((shape1 isClassOf Circle && ((Circle)shape1).radius > 4.0) ||
    (shape2 isClassOf Rectangle && ((Rectangle)shape2).width > 8.0)) {
    print("At least one shape meets size criteria");
}

// Test 6: Casting with early return pattern
print("Test 6: Conditional return with casting");
function checkShapeArea(Shape s): string {
    if (s isClassOf Circle) {
        Circle c = (Circle)s;
        if (c.getArea() > 50.0) {
            return "Large circle";
        }
        return "Small circle";
    }

    if (s isClassOf Rectangle) {
        Rectangle r = (Rectangle)s;
        if (r.getArea() > 50.0) {
            return "Large rectangle";
        }
        return "Small rectangle";
    }

    return "Unknown shape";
}

print(checkShapeArea(new Circle(10.0)));
print(checkShapeArea(new Rectangle(3.0, 4.0)));

// Test 7: Casting in else-if chain with complex logic
print("Test 7: Complex else-if chain");
Shape complexShape = new Rectangle(15.0, 8.0);

if (complexShape isClassOf Circle) {
    print("Circle branch");
} else if (complexShape isClassOf Rectangle && ((Rectangle)complexShape).width > 10.0) {
    print("Wide rectangle: perimeter = " + ((Rectangle)complexShape).getPerimeter());
} else if (complexShape isClassOf Sphere) {
    print("Sphere branch");
} else {
    print("Other shape");
}

print("All conditional casting tests completed");
