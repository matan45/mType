// Test: Casting in switch expression
class Shape {
    public int sides;

    constructor(int s) {
        this.sides = s;
    }
}

class Polygon extends Shape {
    public int internalAngles;

    constructor(int s, int a):super(s) {
        this.internalAngles = a;
    }

    public function getAngleSum(): int {
        return this.internalAngles * this.sides;
    }
}

Shape shape = new Polygon(4, 90);

// Cast in switch expression
switch (((Polygon)shape).getAngleSum()) {
    case 180:
        print("Triangle");
        break;
    case 360:
        print("Quadrilateral");
        break;
    default:
        print("Other polygon");
        break;
}

// Cast to get switch value from object
Shape triangle = new Polygon(3, 60);
int angleSum = ((Polygon)triangle).getAngleSum();
switch (angleSum) {
    case 180:
        print("Correct triangle angles");
        break;
    case 360:
        print("Wrong");
        break;
    default:
        print("Invalid");
        break;
}

// Multiple casts in switch cases
Shape s = new Polygon(5, 108);
switch (((Polygon)s).sides) {
    case 3:
        print("Triangle: " + ((Polygon)s).getAngleSum());
        break;
    case 4:
        print("Quadrilateral: " + ((Polygon)s).getAngleSum());
        break;
    case 5:
        print("Pentagon: " + ((Polygon)s).getAngleSum());
        break;
    default:
        print("Unknown");
        break;
}

// Expected output:
// Quadrilateral
// Correct triangle angles
// Pentagon: 540
