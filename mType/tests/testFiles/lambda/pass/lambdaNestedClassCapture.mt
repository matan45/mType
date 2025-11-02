// Inner class lambda capturing outer this test
interface Function {
    function apply(int x) : int;
}

class Outer {
    int outerValue;

    function init(int val) {
        this.outerValue = val;
    }

    function createAdder() : Function {
        Function adder = x -> {
            // Lambda captures 'this' from Outer class
            return this.outerValue + x;
        };
        return adder;
    }

    function createMultiplier() : Function {
        int localMult = 2;
        Function mult = x -> {
            // Captures both 'this' and local variable
            return this.outerValue * localMult * x;
        };
        return mult;
    }

    function getValue() : int {
        return this.outerValue;
    }

    function setValue(int v) {
        this.outerValue = v;
    }
}

print("=== Nested Class Capture Test ===");

Outer obj = new Outer(10);
Function adder = obj.createAdder();

print("Initial outer value: " + obj.getValue());
print("Adder(5): " + adder.apply(5));
print("Adder(10): " + adder.apply(10));

// Change outer value
obj.setValue(20);
print("After changing to 20:");
print("Adder(5): " + adder.apply(5));

Function mult = obj.createMultiplier();
print("Multiplier(3): " + mult.apply(3));  // 20 * 2 * 3 = 120

print("Nested class capture complete");
