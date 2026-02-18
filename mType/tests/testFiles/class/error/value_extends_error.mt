// Test: Value class cannot extend another class
// Expected error: Value class cannot extend another class

class Base {
    int x;
}

value class InvalidChild extends Base {
    int y;
}
