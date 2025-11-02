// Test method reference type compatibility with lambda interfaces
class Calculator {
    public function double(int x) : int {
        return x * 2;
    }

    public function triple(int x) : int {
        return x * 3;
    }

    static function quadruple(int x) : int {
        return x * 4;
    }
}

interface UnaryOp {
    public function apply(int value) : int;
}

print("=== Lambda Method Reference Type Checking Test ===");

Calculator calc = new Calculator();

// Instance method reference - types must match interface
UnaryOp op1 = x -> calc.double(x);
print("Double 5: " + op1.apply(5));

UnaryOp op2 = x -> calc.triple(x);
print("Triple 5: " + op2.apply(5));

// Static method reference - types must match interface
UnaryOp op3 = x -> Calculator.quadruple(x);
print("Quadruple 5: " + op3.apply(5));

// Chain method calls with type checking
interface ChainedOp {
    public function process(int x) : int;
}

ChainedOp chained = x -> calc.double(calc.triple(x));
print("Triple then double 2: " + chained.process(2));

print("Method reference type checking passed");
