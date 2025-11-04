import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic method inlining optimization candidate
interface UnaryFunction<T> {
    function apply(T value): T;
}

class Math {
    public static function <T> identity(T value): T {
        return value;
    }

    public static function <T> apply(T value, UnaryFunction<T> fn): T {
        return fn.apply(value);
    }
}

function main(): void {
    Int result1 = Math::identity<Int>(new Int(42));
    print("Identity: " + result1.toString());

    Int result2 = Math::apply<Int>(new Int(10), x -> new Int(x.value * 2));
    print("Applied: " + result2.toString());

    String result3 = Math::identity<String>(new String("test"));
    print("Identity: " + result3.toString());
}

main();
