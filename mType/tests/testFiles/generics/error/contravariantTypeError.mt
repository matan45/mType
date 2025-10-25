// Test contravariant type error (if not supported)
import * from "../../lib/collections/List.mt";

class Animal {
    string name;
}

class Dog extends Animal {
    public function Dog(string n) {
        name = n;
    }
}

function main(): void {
    // ERROR: Lower bounds (? super) may not be supported
    List<? super Dog> animals = new List<Animal>();

    print("This should not execute");
}

main();
