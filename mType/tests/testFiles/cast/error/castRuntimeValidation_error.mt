// Error: Test runtime type validation failure
// This test verifies that runtime type checking catches invalid casts
// even when the compile-time types appear compatible

class Shape {
    function area() : float {
        return 0.0;
    }
}

class Rectangle extends Shape {
    int width;
    int height;

    constructor(int w, int h) {
        width = w;
        height = h;
    }

    function area() : float {
        return (float)(width * height);
    }
}

class Circle extends Shape {
    float radius;

    constructor(float r) {
        radius = r;
    }

    function area() : float {
        return 3.14159 * radius * radius;
    }

    function getRadius() : float {
        return radius;
    }
}

// Create a Circle instance
Circle circle = new Circle(5.0);
print("Circle area: " + circle.area());

// Upcast to Shape (valid)
Shape shape = circle;
print("Shape area: " + shape.area());

// Runtime validation should fail: trying to cast Circle to Rectangle
// Even though both extend Shape, Circle is not a Rectangle
Rectangle rect = (Rectangle)shape;  // Error: Runtime type validation failed - cannot cast Circle to Rectangle
print("Rectangle area: " + rect.area());
