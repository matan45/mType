// Test: a downcast to a sibling subclass fails at runtime; the
// exception is catchable as Exception and the program recovers.
import * from "../../lib/exceptions/Exception.mt";

class Animal { public string name; constructor(string n) { name = n; } }
class Cat extends Animal { constructor(string n) : super(n) {} public function purr(): string { return "purr"; } }
class Dog extends Animal { constructor(string n) : super(n) {} public function bark(): string { return "woof"; } }

function describe(Animal a): string {
    try {
        Cat c = (Cat)a;
        return c.purr();
    } catch (Exception e) {
        return "not a cat";
    }
}

function main(): void {
    print(describe(new Cat("Felix")));
    print(describe(new Dog("Rex")));
    print(describe(new Cat("Whiskers")));
}
main();
