import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Basic instance generic method with single type parameter
class Container {
    private int id;

    public constructor(int value) {
        this.id = value;
    }

    // Instance method with generic type parameter
    public function <T> identity(T value): T {
        print("Container " + this.id + " processing: " + value);
        return value;
    }
}

function main(): void {
    Container container = new Container(42);

    // Test with String
    String str = new String("hello");
    String result1 = container.identity<String>(str);
    print("Result 1: " + result1);

    // Test with Int
    Int num = new Int(100);
    Int result2 = container.identity<Int>(num);
    print("Result 2: " + result2);

    print("Basic instance generic test passed");
}

main();
