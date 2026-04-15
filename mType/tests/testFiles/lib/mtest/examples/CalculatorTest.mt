// Example suite adapted from the MYT-59 Jira spec. Demonstrates @Test,
// @BeforeEach, @Disabled, and @Test(expected=...). The Calculator class
// below stands in for "code under test" that would normally live elsewhere.
import * from "../Mtest.mt";

class Calculator {
    public constructor() { }

    public function add(int a, int b): int {
        return a + b;
    }

    public function divide(int a, int b): int {
        if (b == 0) {
            throw new Exception("cannot divide by zero");
        }
        return a / b;
    }

    public function concat(string a, string b): string {
        return a + b;
    }
}

public class CalculatorTest extends TestSuite {
    private Calculator calc;

    public constructor() : super() {
        this.calc = new Calculator();
    }

    @BeforeEach
    public function setUp(): void {
        this.calc = new Calculator();
    }

    @Test
    public function testAdd(): void {
        assertEqual(this.calc.add(2, 2), 4);
    }

    @Test
    public function testAddNegative(): void {
        assertEqual(this.calc.add(-1, 1), 0, "negatives should cancel");
    }

    @Test
    public function testStringConcat(): void {
        assertEqual(this.calc.concat("hello", " world"), "hello world");
    }

    // NOTE: use assertThrows rather than @Test(expected = ...). Exceptions
    // thrown from a reflectively-invoked test body currently get swallowed
    // by the VM's state-restore in Method.invoke (the handler's unwind is
    // overwritten when invokeMethod restores the saved callStack). Wrapping
    // the throwing code in a lambda keeps the throw inside the same
    // invocation frame, where try/catch works correctly.
    @Test
    public function testDivideByZero(): void {
        assertThrows("Exception", () -> {
            int x = this.calc.divide(1, 0);
        });
    }

    @Disabled(reason = "pending feature")
    @Test
    public function testNotYet(): void {
        fail("should not run");
    }
}
