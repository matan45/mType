// Test: Method overloading resolution with inheritance
// Expected: Pass - demonstrates overload resolution in inheritance hierarchy

class Shape {
    public void draw(string color) {
        print("Shape.draw(string): " + color);
    }

    public void draw(string color, int thickness) {
        print("Shape.draw(string, int): " + color + ", thickness: " + thickness);
    }
}

class Circle extends Shape {
    // Overload in child class - different signature
    public void draw(int radius) {
        print("Circle.draw(int): radius = " + radius);
    }

    // Overload in child class - three parameters
    public void draw(string color, int radius, bool filled) {
        print("Circle.draw(string, int, bool): " + color + ", r=" + radius + ", filled=" + filled);
    }
}

class AdvancedCircle extends Circle {
    // Override parent method
    public void draw(int radius) {
        print("AdvancedCircle.draw(int): enhanced radius = " + radius);
    }

    // Additional overload
    public void draw(int radius, int borderWidth) {
        print("AdvancedCircle.draw(int, int): r=" + radius + ", border=" + borderWidth);
    }
}

// Test overload resolution
print("Testing Shape:");
Shape s = new Shape();
s.draw("red");
s.draw("blue", 2);

print("\nTesting Circle:");
Circle c = new Circle();
c.draw("green");
c.draw("yellow", 3);
c.draw(5);
c.draw("purple", 10, true);

print("\nTesting AdvancedCircle:");
AdvancedCircle ac = new AdvancedCircle();
ac.draw("orange");
ac.draw("pink", 2);
ac.draw(7);
ac.draw(8, 3);
ac.draw("cyan", 12, false);
