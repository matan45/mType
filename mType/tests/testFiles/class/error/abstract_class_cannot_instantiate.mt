// Test: Cannot instantiate abstract class
// Expected error: Cannot instantiate abstract class 'Vehicle'

abstract class Vehicle {
    abstract function start(): void;
}

// This should cause an error
Vehicle v = new Vehicle();
