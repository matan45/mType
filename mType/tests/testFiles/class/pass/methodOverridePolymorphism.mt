// Test: Method overriding and polymorphism with inheritance
// Expected: Pass - demonstrates proper method override and polymorphism

class Shape {
    public function draw(string color): void {
        print("Shape.draw: " + color);
    }

    public function getArea(int dimension): int {
        print("Shape.getArea: " + dimension);
        return dimension;
    }

    public function describe():void {
        print("I am a Shape");
    }
}

class Circle extends Shape {
    // Override parent method - same signature
    public function draw(string color): void {
        print("Circle.draw: drawing circle in " + color);
    }

    // Override parent method - same signature
    public function getArea(int radius): int {
        print("Circle.getArea: r=" + radius);
        return radius * radius * 3;  // Simplified PI * r^2
    }

    // Override parent method
    public function describe(): void {
        print("I am a Circle");
    }

    // New method - different name, no conflict
    public function setRadius(int r): int {
        print("Circle.setRadius: " + r);
		return 0;
    }
}

class AdvancedCircle extends Circle {
    // Override grandparent and parent method - same signature
    public function draw(string color): void {
        print("AdvancedCircle.draw: enhanced drawing in " + color);
    }

    // Override parent method
    public function getArea(int radius): int {
        print("AdvancedCircle.getArea: enhanced calculation for r=" + radius);
        return radius * radius * 3 + 1;  // "Enhanced" calculation
    }

    // Override parent method
    public function describe(): void {
        print("I am an AdvancedCircle with special features");
    }

    // New method - different name
    public function setBorderWidth(int width):void {
        print("AdvancedCircle.setBorderWidth: " + width);
    }
}

// Test method overriding and polymorphism
print("Testing Shape:");
Shape s = new Shape();
s.draw("red");
s.getArea(5);
s.describe();

print("\nTesting Circle:");
Circle c = new Circle();
c.draw("green");
c.getArea(10);
c.describe();
c.setRadius(7);

print("\nTesting AdvancedCircle:");
AdvancedCircle ac = new AdvancedCircle();
ac.draw("blue");
ac.getArea(15);
ac.describe();
ac.setRadius(8);
ac.setBorderWidth(2);

print("\nTesting Polymorphism:");
// Polymorphism - Circle object assigned to Shape reference
Shape s2 = new Circle();
s2.draw("yellow");       // Should call Circle.draw (polymorphism)
s2.getArea(20);          // Should call Circle.getArea (polymorphism)
s2.describe();           // Should call Circle.describe (polymorphism)

// Polymorphism - AdvancedCircle object assigned to Shape reference
Shape s3 = new AdvancedCircle();
s3.draw("purple");       // Should call AdvancedCircle.draw (polymorphism)
s3.getArea(25);          // Should call AdvancedCircle.getArea (polymorphism)
s3.describe();           // Should call AdvancedCircle.describe (polymorphism)

// Polymorphism - AdvancedCircle assigned to Circle reference
Circle c2 = new AdvancedCircle();
c2.draw("orange");       // Should call AdvancedCircle.draw (polymorphism)
c2.getArea(30);          // Should call AdvancedCircle.getArea (polymorphism)
c2.describe();           // Should call AdvancedCircle.describe (polymorphism)
c2.setRadius(12);        // Should call Circle.setRadius (AdvancedCircle doesn't override)
