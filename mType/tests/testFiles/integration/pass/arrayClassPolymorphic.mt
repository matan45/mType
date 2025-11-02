// Arrays + Class Test 1: Arrays of polymorphic objects
@Script

class Shape {
    field name: String;

    constructor(n: String) {
        this.name = n;
    }

    fun area(): Float {
        return 0.0;
    }

    fun getName(): String {
        return this.name;
    }

    fun describe(): Void {
        print(this.name);
        print(this.area());
    }
}

class Circle extends Shape {
    field radius: Float;

    constructor(r: Float) {
        super("Circle");
        this.radius = r;
    }

    fun area(): Float {
        return 3.14159 * this.radius * this.radius;
    }
}

class Rectangle extends Shape {
    field width: Float;
    field height: Float;

    constructor(w: Float, h: Float) {
        super("Rectangle");
        this.width = w;
        this.height = h;
    }

    fun area(): Float {
        return this.width * this.height;
    }
}

class Square extends Rectangle {
    constructor(side: Float) {
        super(side, side);
        this.name = "Square";
    }
}

print("Creating array of shapes:");
let shapes: Shape[] = Shape[4];
shapes[0] = Circle(5.0);
shapes[1] = Rectangle(4.0, 6.0);
shapes[2] = Square(3.0);
shapes[3] = Circle(2.0);

print("Iterating polymorphic array:");
let i: Int = 0;
while (i < 4) {
    shapes[i].describe();
    i = i + 1;
}

print("Total area:");
let totalArea: Float = 0.0;
i = 0;
while (i < 4) {
    totalArea = totalArea + shapes[i].area();
    i = i + 1;
}
print(totalArea);

print("Array of circles:");
let circles: Circle[] = Circle[3];
circles[0] = Circle(1.0);
circles[1] = Circle(2.0);
circles[2] = Circle(3.0);

i = 0;
while (i < 3) {
    circles[i].describe();
    i = i + 1;
}
