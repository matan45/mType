class Point {
    float x;
    float y;
    
    constructor(float px, float py) {
        x = px;
        y = py;
    }
    
    function distanceFromOrigin(): float {
        return sqrt(x * x + y * y);
    }
    
    function getCoordinates(): string {
        return "(" + toString(x) + ", " + toString(y) + ")";
    }
    
    static function createOrigin(): Point {
        return new Point(0.0, 0.0);
    }
}

class Shape {
    static int shapeCount = 0;
    static final string category = "2D Shape";
    
    static function getShapeCount(): int {
        return Shape::shapeCount;
    }
    
    static function incrementShapeCount(): void {
        Shape::shapeCount = Shape::shapeCount + 1;
    }
    
    static function getCategory(): string {
        return Shape::category;
    }
}

Point pt = Point::createOrigin();