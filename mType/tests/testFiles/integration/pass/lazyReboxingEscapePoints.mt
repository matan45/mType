// Test: Lazy re-boxing — raw primitives auto-box at escape points
// Verifies that raw int64_t/double results are correctly boxed when they
// escape to object context (method receiver, field storage, collection).
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";

// Test escape via method receiver: arithmetic result used as receiver
function testMethodOnResult(): void {
    Int a = new Int(42);
    Int b = new Int(8);

    // Result of add() is raw int64_t; calling toString() on it requires boxing
    string s = a.add(b).toString();
    print("toString: " + s);

    // Comparison methods on arithmetic results
    bool eq = a.add(b).equals(new Int(50));
    print("equals 50: " + eq);

    int cmp = a.subtract(b).compareTo(new Int(30));
    print("compareTo 30: " + cmp);
}

// Test escape via field storage: arithmetic result stored in object field
class Container {
    public Int stored;

    constructor() {
        this.stored = new Int(0);
    }

    public function setFromArithmetic(Int x, Int y): void {
        this.stored = x.add(y);
    }

    public function getStored(): Int {
        return this.stored;
    }
}

function testFieldStorage(): void {
    Container c = new Container();
    c.setFromArithmetic(new Int(15), new Int(25));
    print("stored: " + c.getStored());
}

// Test escape via print: raw primitive used in string concatenation
function testPrintEscape(): void {
    Int x = new Int(100);
    Int y = new Int(37);
    print("concat: " + x.subtract(y));
}

// Test float lazy re-boxing
function testFloatChain(): void {
    Float a = new Float(3.5);
    Float b = new Float(2.0);

    Float result = a.add(b).multiply(new Float(4.0));
    print("float (3.5+2.0)*4.0 = " + result);

    string fs = a.divide(b).toString();
    print("float toString: " + fs);
}

function main(): void {
    testMethodOnResult();
    testFieldStorage();
    testPrintEscape();
    testFloatChain();
}
main();
