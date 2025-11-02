import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test method overloading with extension-like patterns
class StringHelper {
    // Static methods that extend string functionality
    public static function concat(string a, string b): string {
        return a + b;
    }

    public static function concat(string a, string b, string c): string {
        return a + b + c;
    }

    public static function concat(string[] parts): string {
        string result = "";
        int i = 0;
        while (i < parts.length) {
            result = result + parts[i];
            i = i + 1;
        }
        return result;
    }
}

// Test overloading with wrapper classes
class NumberHelper {
    public static function add(int a, int b): int {
        return a + b;
    }

    public static function add(Int a, Int b): Int {
        return new Int(a.intValue() + b.intValue());
    }

    public static function add(int[] numbers): int {
        int sum = 0;
        int i = 0;
        while (i < numbers.length) {
            sum = sum + numbers[i];
            i = i + 1;
        }
        return sum;
    }
}

function main(): void {
    print("Testing extension-like method overloads");

    // Test StringHelper overloads
    print(StringHelper.concat("Hello", " World"));
    print(StringHelper.concat("A", "B", "C"));

    string[] parts = {"One", "Two", "Three"};
    print(StringHelper.concat(parts));

    // Test NumberHelper overloads
    print(NumberHelper.add(10, 20));

    Int num1 = new Int(5);
    Int num2 = new Int(15);
    Int sum = NumberHelper.add(num1, num2);
    print(sum.intValue());

    int[] numbers = {1, 2, 3, 4, 5};
    print(NumberHelper.add(numbers));

    print("Extension overload test completed");
}

main();
