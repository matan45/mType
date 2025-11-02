// Test static method hiding in inheritance hierarchy
class Base {
    public static function display(int num): string {
        return "Base display: " + num;
    }

    public static function display(string msg): string {
        return "Base display: " + msg;
    }

    public function instanceMethod(int x): string {
        return "Base instance: " + x;
    }
}

class Derived extends Base {
    // Hide base static method with different signature
    public static function display(int num, string msg): string {
        return "Derived display: " + num + ", " + msg;
    }

    // Override instance method
    public function instanceMethod(int x): string {
        return "Derived instance: " + x;
    }

    // Additional overload in derived class
    public function instanceMethod(string msg): string {
        return "Derived instance string: " + msg;
    }
}

function main(): void {
    print("Testing static method hiding and overloads");

    // Call base static methods
    print(Base.display(42));
    print(Base.display("Hello"));

    // Call derived static method
    print(Derived.display(100, "World"));

    // Base static methods still accessible via derived class
    print(Derived.display(99));
    print(Derived.display("Test"));

    // Test instance methods
    Base baseObj = new Base();
    print(baseObj.instanceMethod(10));

    Derived derivedObj = new Derived();
    print(derivedObj.instanceMethod(20));
    print(derivedObj.instanceMethod("Message"));

    // Polymorphic call
    Base polyObj = new Derived();
    print(polyObj.instanceMethod(30));

    print("Static hiding test completed");
}

main();
