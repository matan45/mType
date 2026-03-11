// Test: Multi-parameter resolution with clear winner via parameter-by-parameter comparison
class Animal {
    constructor() {}
}

class Dog extends Animal {
    constructor() {}
}

// Overload 1: exact Dog, exact float
function process(Dog d, float x): string {
    return "Dog, float";
}

// Overload 2: inheritance Dog->Animal, widening int->float
function process(Animal a, float x): string {
    return "Animal, float";
}

function main(): void {
    print("=== Multi Param Mixed Conversions ===");

    Dog d = new Dog();

    // Call process(Dog, float):
    //   Overload 1: (EXACT, EXACT) -> clearly better
    //   Overload 2: (INHERITANCE, EXACT) -> worse on param1
    print(process(d, 3.14));

    // Call process(Dog, int):
    //   Overload 1: (EXACT, WIDENING)
    //   Overload 2: (INHERITANCE, WIDENING)
    //   Overload 1 better on param1, equal on param2 -> wins
    print(process(d, 42));
}
main();
