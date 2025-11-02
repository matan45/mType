import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method inlining optimization candidate
class Math {
    public static function <T> identity(T value): T {
        return value;
    }

    public static function <T> apply(T value, function(T): T fn): T {
        return fn(value);
    }
}

function main(): void {
    Int result1 = Math.identity(new Int(42));
    print("Identity: " + result1);

    Int result2 = Math.apply(new Int(10), function(Int x): Int {
        return new Int(x.value * 2);
    });
    print("Applied: " + result2);

    String result3 = Math.identity(new String("test"));
    print("Identity: " + result3);
}

main();
