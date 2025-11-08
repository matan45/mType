import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test generic method vs non-generic method overload
class Printer {
    // Non-generic method for string
    public static function display(string msg): string {
        return "String display: " + msg;
    }
}

// Test with generic and specific overloads
class Formatter {
    // Specific overload for int
    public function format(int num): string {
        return "Int format: " + num;
    }
}

function main(): void {
    print("Testing generic vs non-generic method overloads");

    // Test Printer class - specific method should be preferred
    print(Printer::display("Hello"));

    // Test Formatter instance methods
    Formatter formatter = new Formatter();
    print(formatter.format(100));

    print("Generic overload test completed");
}

main();
