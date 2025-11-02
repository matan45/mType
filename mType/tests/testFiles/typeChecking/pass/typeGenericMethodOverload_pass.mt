import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test generic method overload resolution
class Box<T> {
    T value;

    constructor(T v) {
        value = v;
    }

    public function getValue(): T {
        return value;
    }
}

// Generic function overloaded with different type parameter counts
function <T> process(T item): string {
    return "Single type: " + item;
}

function <T, U> process(T first, U second): string {
    return "Two types: " + first + ", " + second;
}

// Generic method overload with constraints
function <T> transform(Box<T> box): string {
    return "Transformed box: " + box.getValue();
}

function <T> transform(T item): string {
    return "Transformed item: " + item;
}

// Overload with specific vs generic
class Processor {
    public static function <T> handle(T item): string {
        return "Generic handler: " + item;
    }

    public static function <T> handle(Box<T> box): string {
        return "Box handler: " + box.getValue();
    }
}

function main(): void {
    print("Testing generic method overload resolution");

    // Test single parameter overload
    Int num = new Int(42);
    print(process<Int>(num));

    // Test two parameter overload
    String str = new String("Hello");
    print(process<Int, String>(num, str));

    // Test transform overloads
    Box<Int> box = new Box<Int>(new Int(99));
    print(transform<Int>(box));
    print(transform<Int>(new Int(77)));

    // Test static method overloads
    print(Processor.handle<Int>(new Int(10)));
    Box<String> strBox = new Box<String>(new String("Test"));
    print(Processor.handle<String>(strBox));

    print("Generic method overload test completed");
}

main();
