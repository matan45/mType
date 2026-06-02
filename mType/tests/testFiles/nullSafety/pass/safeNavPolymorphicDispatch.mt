// Test: ?. on a base-typed nullable receiver dispatches virtually to the
// runtime type's override, and short-circuits when null (MYT-374).

class Sound {
    string text;
    constructor(string t) { text = t; }
    public function getText(): string { return text; }
}

class Animal {
    public function makeSound(): Sound { return new Sound("Animal sound"); }
}

class Dog extends Animal {
    public function makeSound(): Sound { return new Sound("Woof"); }
}

class Cat extends Animal {
    public function makeSound(): Sound { return new Sound("Meow"); }
}

function main(): void {
    Animal? a = new Dog();
    Sound? s1 = a?.makeSound();
    if (s1 != null) { print(s1.getText()); }

    Animal? b = new Cat();
    Sound? s2 = b?.makeSound();
    if (s2 != null) { print(s2.getText()); }

    Animal? n = null;
    Sound? s3 = n?.makeSound();
    if (s3 == null) { print("null sound"); }
}

main();

// Expected output:
// Woof
// Meow
// null sound
