// Test casting with promises

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";

print("=== Async Promise Cast Test ===");

class Base {
    int id;

    public constructor(int i) {
        this.id = i;
    }

    public function getId(): int {
        return this.id;
    }

    public function async process(): Promise<String> {
        return new String("Base process");
    }
}

class Derived extends Base {
    string name;

    public constructor(int i, string n):super(i) {
        this.name = n;
    }

    public function getName(): string {
        return this.name;
    }

    public function async process(): Promise<String> {
        return new String("Derived process: " + this.name);
    }
}

function async createDerived(): Promise<Derived> {
    print("Creating derived object");
    Derived d = new Derived(1, "TestObject");
    return d;
}

function async testCasting(): Promise<String> {
    // Get derived object
    Derived derived = await createDerived();
    print("Derived ID: " + derived.getId());
    print("Derived name: " + derived.getName());

    // Upcast to base
    Base base = derived;
    print("Base ID: " + base.getId());

    String result = await base.process();
    print("Result: " + result.getValue());

    return result;
}

function async main(): Promise<String> {
    string result = await testCasting();
    return new String(result);
}

main();
