// Test: Abstract methods cannot have a body
// Expected error: Abstract method must not have a body

abstract class Shape {
    // This should cause a parse error - abstract method with body
    abstract function getArea(): float {
        return 0.0;
    }
}
