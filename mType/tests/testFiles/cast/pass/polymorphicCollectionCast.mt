// Test: collection of polymorphic elements; iterate, runtime-type-check, and
// downcast each element to its concrete type to invoke subtype-only methods.
class Animal {
    public string name;
    public constructor(string n) { this.name = n; }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
    public function fetch(): void { print(this.name + " fetches"); }
}

class Cat extends Animal {
    public constructor(string n) : super(n) {}
    public function purr(): void { print(this.name + " purrs"); }
}

Animal[] animals = new Animal[3];
animals[0] = new Dog("Rex");
animals[1] = new Cat("Whiskers");
animals[2] = new Dog("Buddy");

for (Animal a : animals) {
    if (a isClassOf Dog) {
        Dog d = (Dog)a;
        d.fetch();
    } else if (a isClassOf Cat) {
        Cat c = (Cat)a;
        c.purr();
    }
}

// Expected output:
// Rex fetches
// Whiskers purrs
// Buddy fetches
