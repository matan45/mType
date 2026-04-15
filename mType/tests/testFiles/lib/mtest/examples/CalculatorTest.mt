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

    public constructor() : super() { }

    // @BeforeEach owns per-test initialization. TestRunner constructs a
    // fresh suite instance before each @Test and invokes this hook, so
    // the constructor body intentionally does no field setup.
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

    @Test(expected = "Exception")
    public function testDivideByZero(): void {
        int x = this.calc.divide(1, 0);
    }

    @Disabled(reason = "pending feature")
    @Test
    public function testNotYet(): void {
        fail("should not run");
    }
}
