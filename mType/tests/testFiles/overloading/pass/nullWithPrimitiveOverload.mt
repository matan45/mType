// Test: Overload resolution with object type and primitive type
class Animal {
    string name;

    constructor(string n) {
        this.name = n;
    }

    public function getName(): string {
        return this.name;
    }
}

// Overloads with different parameter types
function check(Animal a): string {
    return "Animal: " + a.getName();
}

function check(int x): string {
    return "int: " + x;
}

function check(string s): string {
    return "string: " + s;
}

function main(): void {
    print("=== Object vs Primitive Overload ===");

    Animal a = new Animal("dog");
    print(check(a));       // exact Animal match
    print(check(42));      // exact int match
    print(check("hello")); // exact string match
}
main();
