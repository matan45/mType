// Test: Type check before cast
class Shape {}
class Circle extends Shape { int radius = 5; }
class Square extends Shape { int side = 4; }

Shape shape = new Circle();

if (shape isClassOf Circle) {
    Circle c = (Circle)shape;
    print(c.radius);
} else if (shape isClassOf Square) {
    Square s = (Square)shape;
    print(s.side);
}

// Expected output:
// 5
