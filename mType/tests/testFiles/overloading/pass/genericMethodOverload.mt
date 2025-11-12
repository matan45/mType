import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic method overloading in classes
class Container {
    // Default constructor
    public constructor() {
    }

    // Generic method for any type
    public function <T> describe(T item): string {
        return "Generic item: " + item;
    }

    // Generic method with two type parameters
    public function <T, K> describe(T first, K second): string {
        return "Two items: " + first + ", " + second;
    }

    // Specific overload for integers
    public function describe(int item): string {
        return "Integer: " + item;
    }

    // Specific overload for strings
    public function describe(string item): string {
        return "String: " + item;
    }
}


function main(): void {
    Container c = new Container();

    string result1 = c.describe(42);              // Should call specific int version
    print(result1);

    string result2 = c.describe("Hello");         // Should call specific String version
    print(result2);

    string result3 = c.describe<Bool>(true);            // Should call generic T version
    print(result3);

    string result4 = c.describe<Int,String>(10, "World");     // Should call generic T, K version
    print(result4);
}
main();