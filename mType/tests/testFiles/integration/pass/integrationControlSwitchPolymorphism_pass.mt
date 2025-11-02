// Test: Polymorphism in switch statements and control flow
// Tests polymorphic behavior with type checking in switch/case patterns

class Shape {
    private int shapeType;

    public constructor(int type) {
        this.shapeType = type;
    }

    public int getType(): int {
        return this.shapeType;
    }

    public string getName(): string {
        return "Shape";
    }
}

class Circle {
    private int radius;
    private int typeId;

    public constructor(int r) {
        this.radius = r;
        this.typeId = 1;
    }

    public int getType(): int {
        return this.typeId;
    }

    public string getName(): string {
        return "Circle";
    }

    public int getRadius(): int {
        return this.radius;
    }

    public int area(): int {
        return 3 * this.radius * this.radius;
    }
}

class Rectangle {
    private int width;
    private int height;
    private int typeId;

    public constructor(int w, int h) {
        this.width = w;
        this.height = h;
        this.typeId = 2;
    }

    public int getType(): int {
        return this.typeId;
    }

    public string getName(): string {
        return "Rectangle";
    }

    public int getWidth(): int {
        return this.width;
    }

    public int getHeight(): int {
        return this.height;
    }

    public int area(): int {
        return this.width * this.height;
    }
}

class Triangle {
    private int base;
    private int height;
    private int typeId;

    public constructor(int b, int h) {
        this.base = b;
        this.height = h;
        this.typeId = 3;
    }

    public int getType(): int {
        return this.typeId;
    }

    public string getName(): string {
        return "Triangle";
    }

    public int getBase(): int {
        return this.base;
    }

    public int getHeight(): int {
        return this.height;
    }

    public int area(): int {
        return (this.base * this.height) / 2;
    }
}

// Test 1: Switch with polymorphic type handling
print("Test 1: Switch with polymorphic types");

function describeShapeType(int shapeType): string {
    string description = "";

    if (shapeType == 0) {
        description = "Generic shape";
    } else {
        if (shapeType == 1) {
            description = "Circle - round shape";
        } else {
            if (shapeType == 2) {
                description = "Rectangle - four sides";
            } else {
                if (shapeType == 3) {
                    description = "Triangle - three sides";
                } else {
                    description = "Unknown shape";
                }
            }
        }
    }

    return description;
}

print(describeShapeType(0));
print(describeShapeType(1));
print(describeShapeType(2));
print(describeShapeType(3));
print(describeShapeType(99));

// Test 2: Control flow with polymorphic behavior
print("\nTest 2: Polymorphic behavior in control flow");

Circle circle = new Circle(5);
Rectangle rect = new Rectangle(4, 6);
Triangle tri = new Triangle(8, 3);

print(circle.getName() + " area: " + circle.area());
print(rect.getName() + " area: " + rect.area());
print(tri.getName() + " area: " + tri.area());

// Test 3: Type-based dispatch in loops
print("\nTest 3: Type-based dispatch in loops");

class ShapeProcessor {
    public void processCircle(Circle c) {
        print("Processing circle with radius: " + c.getRadius());
    }

    public void processRectangle(Rectangle r) {
        print("Processing rectangle " + r.getWidth() + "x" + r.getHeight());
    }

    public void processTriangle(Triangle t) {
        print("Processing triangle base:" + t.getBase() + " height:" + t.getHeight());
    }

    public void processShape(int shapeType, int param1, int param2) {
        if (shapeType == 1) {
            Circle c = new Circle(param1);
            this.processCircle(c);
        } else {
            if (shapeType == 2) {
                Rectangle r = new Rectangle(param1, param2);
                this.processRectangle(r);
            } else {
                if (shapeType == 3) {
                    Triangle t = new Triangle(param1, param2);
                    this.processTriangle(t);
                }
            }
        }
    }
}

ShapeProcessor processor = new ShapeProcessor();
int[] types = new int[3];
types[0] = 1;
types[1] = 2;
types[2] = 3;

int i = 0;
while (i < 3) {
    processor.processShape(types[i], 5 + i, 10 + i);
    i = i + 1;
}

// Test 4: Polymorphic comparison in conditionals
print("\nTest 4: Polymorphic comparison");

class ShapeComparator {
    public string compare(int area1, int area2): string {
        if (area1 > area2) {
            return "First shape is larger";
        } else {
            if (area1 < area2) {
                return "Second shape is larger";
            } else {
                return "Shapes have equal area";
            }
        }
    }
}

ShapeComparator comparator = new ShapeComparator();
Circle c1 = new Circle(3);
Rectangle r1 = new Rectangle(5, 5);

print(comparator.compare(c1.area(), r1.area()));

// Test 5: Switch-like behavior with type categories
print("\nTest 5: Type categories in control flow");

class ShapeCategory {
    public string categorize(int shapeType): string {
        string category = "";

        if (shapeType >= 0 && shapeType <= 1) {
            category = "Curved shapes";
        } else {
            if (shapeType >= 2 && shapeType <= 10) {
                category = "Polygons";
            } else {
                category = "Other shapes";
            }
        }

        return category;
    }

    public string getSides(int shapeType): string {
        string sides = "";

        if (shapeType == 0) {
            sides = "Variable sides";
        } else {
            if (shapeType == 1) {
                sides = "No sides (curved)";
            } else {
                if (shapeType == 2) {
                    sides = "4 sides";
                } else {
                    if (shapeType == 3) {
                        sides = "3 sides";
                    } else {
                        sides = "Unknown";
                    }
                }
            }
        }

        return sides;
    }
}

ShapeCategory categorizer = new ShapeCategory();
i = 0;
while (i < 4) {
    print("Type " + i + ": " + categorizer.categorize(i) + " - " + categorizer.getSides(i));
    i = i + 1;
}

// Test 6: Nested polymorphic dispatch
print("\nTest 6: Nested polymorphic dispatch");

class ShapeFactory {
    public Circle createCircle(int r): Circle {
        return new Circle(r);
    }

    public Rectangle createRectangle(int w, int h): Rectangle {
        return new Rectangle(w, h);
    }

    public Triangle createTriangle(int b, int h): Triangle {
        return new Triangle(b, h);
    }

    public string createAndDescribe(int type, int param1, int param2): string {
        string result = "";

        if (type == 1) {
            Circle c = this.createCircle(param1);
            result = c.getName() + " with area " + c.area();
        } else {
            if (type == 2) {
                Rectangle r = this.createRectangle(param1, param2);
                result = r.getName() + " with area " + r.area();
            } else {
                if (type == 3) {
                    Triangle t = this.createTriangle(param1, param2);
                    result = t.getName() + " with area " + t.area();
                } else {
                    result = "Unknown type";
                }
            }
        }

        return result;
    }
}

ShapeFactory factory = new ShapeFactory();
print(factory.createAndDescribe(1, 4, 0));
print(factory.createAndDescribe(2, 5, 6));
print(factory.createAndDescribe(3, 10, 8));

print("\nSwitch polymorphism tests completed");
