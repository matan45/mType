// Complex overloading scenarios
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";

// Scenario 1: Same method name, varying parameter counts
class Calculator {
    public function add(int a): int {
        return a;
    }

    public function add(int a, int b): int {
        return a + b;
    }

    public function add(int a, int b, int c): int {
        return a + b + c;
    }

    public function add(int a, int b, int c, int d): int {
        return a + b + c + d;
    }
}

// Scenario 2: Mixed primitive and generic overloads
class Processor<T> {
    public function handle(int x): string {
        return "handle(int): " + x;
    }

    public function handle(string s): string {
        return "handle(string): " + s;
    }

    public function handle(bool b): string {
        return "handle(bool): " + b;
    }

    public function handle(T item): string {
        return "handle(T): generic";
    }
}

// Scenario 3: Overload chains with type widening
class TypeWidener {
    public static function convert(int x): string {
        return "int: " + x;
    }

    public static function convert(float x): string {
        return "float: " + x;
    }

    public static function convert(string x): string {
        return "string: " + x;
    }
}

// Scenario 4: Generic methods with constraints simulation
class Container<T> {
    T value;

    constructor(T v) {
        this.value = v;
    }

    // Specific overload for Int
    public function compare(Container<Int> other): string {
        return "Comparing Container<Int>";
    }

    // Specific overload for String
    public function compare(Container<String> other): string {
        return "Comparing Container<String>";
    }

    // Generic fallback
    public function <U> compare(Container<U> other): string {
        return "Comparing Container<U>";
    }
}

// Scenario 5: Multiple dispatch simulation
class Shape {
    public function intersects(Shape other): string {
        return "Shape-Shape";
    }
}

class Circle extends Shape {
    public function intersects(Shape other): string {
        return "Circle-Shape";
    }

    public function intersects(Circle other): string {
        return "Circle-Circle";
    }
}

class Rectangle extends Shape {
    public function intersects(Shape other): string {
        return "Rectangle-Shape";
    }

    public function intersects(Rectangle other): string {
        return "Rectangle-Rectangle";
    }

    public function intersects(Circle other): string {
        return "Rectangle-Circle";
    }
}

// Scenario 6: Overloading with multiple generic class parameters
class BiContainer<T1, T2> {
    T1 first;
    T2 second;

    constructor(T1 f, T2 s) {
        this.first = f;
        this.second = s;
    }
}

function merge(BiContainer<Int, Int> bc): string {
    return "BiContainer<Int,Int>";
}

function merge(BiContainer<String, String> bc): string {
    return "BiContainer<String,String>";
}

function merge(BiContainer<Int, String> bc): string {
    return "BiContainer<Int,String>";
}

function merge(BiContainer<String, Int> bc): string {
    return "BiContainer<String,Int>";
}

function <T> merge(BiContainer<T, T> bc): string {
    return "BiContainer<T,T>: same type";
}

function <T1, T2> merge(BiContainer<T1, T2> bc): string {
    return "BiContainer<T1,T2>: different types";
}

function main(): void {
    print("=== Scenario 1: Varying Parameter Counts ===");
    Calculator calc = new Calculator();
    print("add(5) = " + calc.add(5));
    print("add(3, 7) = " + calc.add(3, 7));
    print("add(1, 2, 3) = " + calc.add(1, 2, 3));
    print("add(1, 2, 3, 4) = " + calc.add(1, 2, 3, 4));

    print("");
    print("=== Scenario 2: Mixed Primitive and Generic ===");
    Processor<Float> proc = new Processor<Float>();
    print(proc.handle(42));
    print(proc.handle("test"));
    print(proc.handle(true));
    print(proc.handle(3.14));

    print("");
    print("=== Scenario 3: Type Widening ===");
    print(TypeWidener::convert(10));
    print(TypeWidener::convert(3.14));
    print(TypeWidener::convert("hello"));

    print("");
    print("=== Scenario 4: Generic Constraints Simulation ===");
    Container<Int> intC1 = new Container<Int>(1);
    Container<Int> intC2 = new Container<Int>(2);
    Container<String> strC = new Container<String>("text");
    Container<Bool> boolC = new Container<Bool>(true);

    print(intC1.compare(intC2));
    print(strC.compare(strC));
    print(intC1.compare<Bool>(boolC));

    print("");
    print("=== Scenario 5: Multiple Dispatch Simulation ===");
    Shape s = new Shape();
    Circle c = new Circle();
    Rectangle r = new Rectangle();

    print(s.intersects(s));
    print(c.intersects(s));
    print(c.intersects(c));
    print(r.intersects(s));
    print(r.intersects(r));
    print(r.intersects(c));

    print("");
    print("=== Scenario 6: Multi-Parameter Generic Overloading ===");
    BiContainer<Int, Int> bc1 = new BiContainer<Int, Int>(1, 2);
    BiContainer<String, String> bc2 = new BiContainer<String, String>("a", "b");
    BiContainer<Int, String> bc3 = new BiContainer<Int, String>(3, "c");
    BiContainer<String, Int> bc4 = new BiContainer<String, Int>("d", 4);
    BiContainer<Bool, Bool> bc5 = new BiContainer<Bool, Bool>(true, false);
    BiContainer<Int, Bool> bc6 = new BiContainer<Int, Bool>(5, true);

    print(merge(bc1));  // Exact: BiContainer<Int,Int>
    print(merge(bc2));  // Exact: BiContainer<String,String>
    print(merge(bc3));  // Exact: BiContainer<Int,String>
    print(merge(bc4));  // Exact: BiContainer<String,Int>
    print(merge<Bool>(bc5));  // Generic same: BiContainer<T,T>
    print(merge<Int, Bool>(bc6));  // Generic different: BiContainer<T1,T2>

    print("");
    print("All complex scenarios passed!");
}

main();
