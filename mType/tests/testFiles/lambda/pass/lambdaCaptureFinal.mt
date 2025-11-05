// Capturing final/const variables test
interface Function {
    function apply(int x) : int;
}

class ConstCapture {
    public function createAdder(int constant) : Function {
        // constant is effectively final (parameter)
        Function adder = x -> constant + x;
        return adder;
    }

    public function complexCapture() : Function {
        int base = 100;  // Local variable captured
        int multiplier = 2;  // Another local

        Function complex = x -> {
            // All captured variables are effectively final
            return (base + x) * multiplier;
        };

        // Note: base and multiplier cannot be modified after lambda creation
        return complex;
    }
}

print("=== Capture Final Test ===");

ConstCapture cc = new ConstCapture();

Function add10 = cc.createAdder(10);
Function add50 = cc.createAdder(50);

print("add10(5) = " + add10.apply(5));
print("add50(5) = " + add50.apply(5));

Function complex = cc.complexCapture();
print("complex(10) = " + complex.apply(10));
print("complex(25) = " + complex.apply(25));

// Multiple lambdas capturing same final variable
int shared = 1000;
Function lambda1 = x -> shared + x;
Function lambda2 = x -> shared - x;
Function lambda3 = x -> shared * x;

print("lambda1(100) = " + lambda1.apply(100));
print("lambda2(100) = " + lambda2.apply(100));
print("lambda3(5) = " + lambda3.apply(5));

print("Capture final complete");
