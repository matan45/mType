// Test: a custom exception subclass carries a boxed Int payload; the
// catch handler unboxes it via getValue() to read the original input.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class BadValueException extends Exception {
    public Int badValue;
    public constructor(string m, Int v) : super(m) {
        badValue = v;
    }
}

function check(Int n): void {
    if (n.getValue() == 0) {
        throw new BadValueException("zero", n);
    }
}

function main(): void {
    try { check(new Int(5)); print("ok 5"); } catch (BadValueException e) { print("nope"); }
    try {
        check(new Int(0));
        print("nope");
    } catch (BadValueException e) {
        print("bad: " + e.badValue.getValue() + " msg: " + e.getMessage());
    }
}
main();
