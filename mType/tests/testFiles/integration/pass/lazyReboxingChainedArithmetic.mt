// Test: Lazy re-boxing — chained arithmetic stays as raw primitives
// Verifies that INVOKE_INT_* opcodes push raw int64_t and chain correctly
// without intermediate boxing/unboxing.
import * from "../../lib/primitives/Int.mt";

function main(): void {
    Int a = new Int(10);
    Int b = new Int(3);
    Int c = new Int(7);
    Int d = new Int(2);

    // Chained method calls: each returns raw int64_t, next call unboxes it
    Int result1 = a.add(b).multiply(c).subtract(d);
    print("(10+3)*7-2 = " + result1);

    // Nested chains
    Int result2 = a.multiply(b).add(c.multiply(d));
    print("10*3 + 7*2 = " + result2);

    // Chain with negate and abs
    Int neg = a.negate();
    Int absNeg = neg.abs();
    print("abs(negate(10)) = " + absNeg);

    // Division and modulo chains
    Int result3 = a.add(b).divide(c).add(d);
    print("(10+3)/7+2 = " + result3);

    Int result4 = a.modulo(b).add(c);
    print("10%3+7 = " + result4);
}
main();
