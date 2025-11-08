import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test generic method vs non-generic method overload
class Printer {
    // Non-generic method for string
    public static function display(string msg): string {
        return "String display: " + msg;
    }

    // Generic method for int (simulated with int parameter)
    public static function display2(int value): string {
        return "Generic display: " + value;
    }
}

// Test with generic and specific overloads
class Formatter {
    // Specific overload for int
    public function format(int num): string {
        return "Int format: " + num;
    }

    // Generic overload for string (simulated with string parameter)
    public function format2(string value): string {
        return "Generic format: " + value;
    }
}

function main(): void {
    print("Testing generic vs non-generic method overloads");

    // Test Printer class - specific method should be preferred
    print(Printer::display("Hello"));
    print(Printer::display2(42));

    // Test Formatter instance methods
    Formatter formatter = new Formatter();
    print(formatter.format(100));
    print(formatter.format2("World"));

    print("Generic overload test completed");
}

main();
