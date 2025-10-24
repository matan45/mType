// Test: Concrete class must implement all abstract methods
// Expected error: Concrete class 'Square' must implement all abstract methods

abstract class Shape {
    abstract function getArea(): float;
    abstract function getPerimeter(): float;
}

// This class only implements getArea but not getPerimeter
class Square extends Shape {
    private float side;

    constructor(float s) {
        side = s;
    }

    function getArea(): float {
        return side * side;
    }

    // Missing: getPerimeter() implementation
}

// This should fail during class registration
Square sq = new Square(5.0);
