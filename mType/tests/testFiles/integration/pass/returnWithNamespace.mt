class Point {
    float x;
    float y;

    public constructor(float px, float py) {
        x = px;
        y = py;
    }

    public function distanceFromOrigin(): float {
        return sqrt(x * x + y * y);
    }

    public function getCoordinates(): string {
        return "(" + x + ", " + y + ")";
    }

    public static function createOrigin(): Point {
        return new Point(0.0, 0.0);
    }
}

class Shape {
    static int shapeCount = 0;
    static final string category = "2D Shape";
    
    static function getShapeCount(): int {
        return shapeCount;
    }
    
    static function incrementShapeCount(): void {
        shapeCount++;
    }
    
    static function getCategory(): string {
        return category;
    }
}

Point pt = Point::createOrigin();