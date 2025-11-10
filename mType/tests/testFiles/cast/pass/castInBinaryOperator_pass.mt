// Test: Casting in binary operator expressions
class Number {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

class ScalableNumber extends Number {
    public int scale;

    constructor(int v, int s):super(v) {
        this.scale = s;
    }

    public function getScaled(): int {
        return this.value * this.scale;
    }
}

Number n1 = new ScalableNumber(10, 2);
Number n2 = new ScalableNumber(5, 3);

// Cast in arithmetic operations
int sum = ((ScalableNumber)n1).getScaled() + ((ScalableNumber)n2).getScaled();
print(sum);  // 20 + 15 = 35

int diff = ((ScalableNumber)n1).getScaled() - ((ScalableNumber)n2).getScaled();
print(diff);  // 20 - 15 = 5

int product = ((ScalableNumber)n1).scale * ((ScalableNumber)n2).scale;
print(product);  // 2 * 3 = 6

// Cast in comparison operations
if (((ScalableNumber)n1).getScaled() > ((ScalableNumber)n2).getScaled()) {
    print("n1 is larger");
} else {
    print("n2 is larger or equal");
}

// Cast in logical operations
Number n3 = new ScalableNumber(7, 2);
if (((ScalableNumber)n1).scale > 1 && ((ScalableNumber)n3).scale > 1) {
    print("Both have scale > 1");
}

// Cast with primitive casting in binary operation
float ratio = (float)((ScalableNumber)n1).getScaled() / (float)((ScalableNumber)n2).getScaled();
print(ratio);  // 20.0 / 15.0 = 1.333...

// Cast in string concatenation
string result = "Total: " + ((ScalableNumber)n1).getScaled() + " + " + ((ScalableNumber)n2).getScaled();
print(result);

// Expected output:
// 35
// 5
// 6
// n1 is larger
// Both have scale > 1
// 1.3333333333333333
// Total: 20 + 15
