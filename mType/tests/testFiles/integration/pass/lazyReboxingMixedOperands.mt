// Test: Lazy re-boxing — mixed boxed and unboxed operands
// Verifies that arithmetic and comparison operators correctly handle
// mixed scenarios: raw primitives, ValueObjects, and ObjectInstances.
import * from "../../lib/primitives/Int.mt";

function main(): void {
    // Mixed: boxed Int with raw int in arithmetic operators
    Int boxed = new Int(10);
    int raw = 5;
    int result1 = raw + raw;
    print("raw+raw: " + result1);

    // Boxed used in raw comparison (auto-unboxing in GT/LT)
    Int a = new Int(20);
    Int b = new Int(15);
    bool gt = a > b;
    print("20 > 15: " + gt);

    bool lt = b < a;
    print("15 < 20: " + lt);

    // Chain that mixes INVOKE_INT_* (returns raw) with raw arithmetic
    Int x = new Int(8);
    Int y = new Int(3);
    Int sum = x.add(y);
    // sum is raw int64_t from lazy re-boxing; using it in another add
    Int total = sum.add(new Int(9));
    print("8+3+9 = " + total);

    // Equality between raw result and boxed value
    Int eq1 = x.add(y);
    Int eq2 = new Int(11);
    bool isEqual = eq1.equals(eq2);
    print("8+3 == 11: " + isEqual);

    // Raw result in compareTo
    int cmp = x.subtract(y).compareTo(new Int(5));
    print("8-3 compareTo 5: " + cmp);

    // Arithmetic result used in a loop condition
    Int counter = new Int(0);
    Int limit = new Int(5);
    int loopCount = 0;
    while (counter < limit) {
        counter = counter.add(new Int(1));
        loopCount = loopCount + 1;
    }
    print("loop count: " + loopCount);
}
main();
