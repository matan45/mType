import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
// Integration test - overloading with classes, methods, static methods, and generics
class Box<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    // Overloaded methods
    public function describe(): string {
        return "Box with value";
    }

    public function describe(string prefix): string {
        return prefix + ": " + this.value;
    }

    public function describe(string prefix, string suffix): string {
        return prefix + " " + this.value + " " + suffix;
    }

    // Static factory methods (overloaded)
    public static function createInt(int val): Box<Int> {
        return new Box<Int>(val);
    }

    public static function createString(string val): Box<String> {
        return new Box<String>(val);
    }

    public static function <U> create(U val): Box<U> {
        return new Box<U>(val);
    }
}

// Global overloaded functions
function processBox(Box<Int> box): void {
    print("Processing Int box: " + box.getValue());
}

function processBox(Box<String> box): void {
    print("Processing String box: " + box.getValue());
}

function <T> processBox(Box<T> box): void {
    print("Processing generic box: " + box.getValue());
}


function main(): void {
    // Constructor overloading (Box has one constructor)
    Box<Int> intBox = new Box<Int>(42);
    Box<String> strBox = new Box<String>("Hello");

    // Instance method overloading
    print(intBox.describe());                     // describe()
    print(intBox.describe("Value"));              // describe(String)
    print(intBox.describe("Start", "End"));       // describe(String, String)

    // Static method overloading
    Box<Int> box1 = Box::createInt(100);            // createInt(int)
    Box<String> box2 = Box::createString("World");  // createString(String)
    Box<Bool> box3 = Box::create(true);             // create<T>(T)

    // Function overloading with generic type arguments
    processBox(box1);   // Should call processBox(Box<Int>)
    processBox(box2);   // Should call processBox(Box<String>)
    processBox<Box<Bool>>(box3);   // Should call processBox<T>(Box<T>)

    print("All overload tests passed!");
}
main();
