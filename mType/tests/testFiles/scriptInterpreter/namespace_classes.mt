// Graphics namespace with classes
namespace graphics {
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
        
        static function createOrigin(): graphics::Point {
            return new graphics::Point(0.0, 0.0);
        }
    }
    
    class Shape {
        static int shapeCount = 0;
        static final string category = "2D Shape";
        
        static function getShapeCount(): int {
            return graphics::Shape::shapeCount;
        }
        
        static function incrementShapeCount(): void {
            graphics::Shape::shapeCount = graphics::Shape::shapeCount + 1;
        }
        
        static function getCategory(): string {
            return graphics::Shape::category;
        }
    }
}

// Math namespace with utility classes
namespace math {
    class Vector {
        float x;
        float y;
        float z;
        
        constructor(float vx, float vy, float vz) {
            x = vx;
            y = vy;
            z = vz;
        }
        
        function magnitude(): float {
            return sqrt(x * x + y * y + z * z);
        }
        
        static function zero(): math::Vector {
            return new math::Vector(0.0, 0.0, 0.0);
        }
    }
}