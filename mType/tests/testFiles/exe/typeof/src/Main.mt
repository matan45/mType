// Test: isClassOf operator in standalone exe

class Animal {
    public string name;

    public constructor(string n) {
        this.name = n;
    }

    public function speak(): string {
        return this.name + " makes a sound";
    }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {
    }

    public function speak(): string {
        return this.name + " barks";
    }

    public function fetch(): string {
        return this.name + " fetches the ball";
    }
}

class Cat extends Animal {
    public constructor(string n) : super(n) {
    }

    public function speak(): string {
        return this.name + " meows";
    }

    public function purr(): string {
        return this.name + " purrs";
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Basic isClassOf
        Dog dog = new Dog("Rex");
        print("dog isClassOf Dog: " + (dog isClassOf Dog));
        print("dog isClassOf Animal: " + (dog isClassOf Animal));
        print("dog isClassOf Cat: " + (dog isClassOf Cat));

        // Polymorphic reference
        Animal animal = new Cat("Whiskers");
        print("animal isClassOf Cat: " + (animal isClassOf Cat));
        print("animal isClassOf Dog: " + (animal isClassOf Dog));
        print("animal isClassOf Animal: " + (animal isClassOf Animal));

        // isClassOf + cast pattern
        if (animal isClassOf Cat) {
            Cat cat = (Cat)animal;
            print(cat.purr());
        }

        // Array of mixed types
        Animal[] animals = new Animal[3];
        animals[0] = new Dog("Buddy");
        animals[1] = new Cat("Luna");
        animals[2] = new Dog("Max");

        int dogCount = 0;
        int catCount = 0;
        for (int i = 0; i < 3; i = i + 1) {
            if (animals[i] isClassOf Dog) {
                dogCount = dogCount + 1;
            } else if (animals[i] isClassOf Cat) {
                catCount = catCount + 1;
            }
        }
        print("Dogs: " + dogCount);
        print("Cats: " + catCount);

        print("TypeOf test passed");
    }
}
