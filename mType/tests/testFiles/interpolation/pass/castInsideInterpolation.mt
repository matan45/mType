// Edge Case: Casting expression inside string interpolation

class Animal {
    public function speak(): string { return "..."; }
}

class Dog extends Animal {
    public function speak(): string { return "Woof"; }
    public function fetch(): string { return "Ball fetched!"; }
}

class Cat extends Animal {
    public function speak(): string { return "Meow"; }
    public function purr(): string { return "Purrrr"; }
}

function main(): void {
    print("=== Edge: Cast Inside Interpolation ===");

    // Basic cast in interpolation
    Animal a = new Dog();
    print($"Dog says: {((Dog)a).speak()}");
    print($"Dog fetches: {((Dog)a).fetch()}");

    // Cast with instanceof check + interpolation
    Animal[] animals = [new Dog(), new Cat(), new Dog()];
    for (int i = 0; i < animals.length; i++) {
        Animal animal = animals[i];
        if (animal isClassOf Dog) {
            Dog d = (Dog)animal;
            print($"Animal {i} is a dog: {d.fetch()}");
        } else if (animal isClassOf Cat) {
            Cat c = (Cat)animal;
            print($"Animal {i} is a cat: {c.purr()}");
        }
    }

    // Primitive casts in interpolation
    print("--- Primitive casts ---");
    float pi = 3.14159;
    print($"Pi as int: {(int)pi}");
    print($"Pi as float: {pi}");

    int num = 42;
    print($"Int as float: {(float)num}");

    bool flag = true;
    print($"Bool to string: {flag}");

    // Chained cast in interpolation
    print("--- Cast chain ---");
    Animal a2 = new Cat();
    if (a2 isClassOf Cat) {
        print($"Cat purrs: {((Cat)a2).purr()}");
    }

    // Null-safe pattern
    print("--- Null check ---");
    Animal? maybeNull = null;
    if (maybeNull != null) {
        print($"Has value: {maybeNull.speak()}");
    } else {
        print("Animal is null");
    }

    print("=== Edge Complete ===");
}

main();
