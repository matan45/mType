import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test generic method vs non-generic method overload
class Printer {
    // Non-generic method for string
    public static function display(string msg): string {
        return "String display: " + msg;
    }

    // Generic method for other types
    public static function <T> display(T item): string {
        return "Generic display: " + item;
    }
}

// Test with generic and specific overloads
class Formatter {
    // Specific overload for int
    public function format(int num): string {
        return "Int format: " + num;
    }

    // Generic overload for other types
    public function <T> format(T item): string {
        return "Generic format: " + item;
    }
}

function main(): void {
    print("Testing generic vs non-generic method overloads");

    // Test Printer class - specific method should be preferred
    print(Printer.display("Hello"));

    // Test Printer with generic type
    Int num = new Int(42);
    print(Printer.display<Int>(num));

    // Test Formatter instance methods
    Formatter formatter = new Formatter();
    print(formatter.format(100));

    String str = new String("World");
    print(formatter.format<String>(str));

    print("Generic overload test completed");
}

main();
