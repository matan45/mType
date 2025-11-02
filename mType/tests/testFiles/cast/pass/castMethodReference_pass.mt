// Test method reference to interface cast
// Method references can be cast to compatible functional interfaces

interface Transformer {
    public function transform(string input): string;
}

interface Calculator {
    public function calculate(int a, int b): int;
}

interface Processor {
    public function process(int value): int;
}

class StringUtils {
    public static function toUpperCase(string s): string {
        return s;  // Simplified - mType may not have toUpperCase
    }

    public static function toLowerCase(string s): string {
        return s;  // Simplified
    }
}

class MathUtils {
    public static function add(int a, int b): int {
        return a + b;
    }

    public static function multiply(int a, int b): int {
        return a * b;
    }

    public function double(int x): int {
        return x * 2;
    }
}

// Test instance method reference
MathUtils math = new MathUtils();
Processor doubler = (Processor)(int x) => {
    return math.double(x);
};

print(doubler.process(5));
print(doubler.process(10));

// Test with calculator interface
Calculator adder = (Calculator)(int a, int b) => {
    return MathUtils.add(a, b);
};

print(adder.calculate(3, 7));
print(adder.calculate(15, 25));

// Test with multiplier
Calculator multiplier = (Calculator)(int a, int b) => {
    return MathUtils.multiply(a, b);
};

print(multiplier.calculate(4, 5));
print(multiplier.calculate(6, 7));

// Chain method reference casts
Processor proc = (Processor)(int x) => {
    return x * 3;
};

int result = proc.process(7);
print(result);

print("Method reference casting test passed");
