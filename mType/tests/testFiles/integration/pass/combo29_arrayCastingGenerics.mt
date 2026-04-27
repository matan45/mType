// Combo 29: ArrayList<Animal> with subtypes + casting + try/catch on bad casts.
// Generic angle is preserved via ArrayList<Animal>; casts are inline because
// `isClassOf T` against a type parameter doesn't work at runtime due to erasure.

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/exceptions/Exception.mt";

abstract class Animal {
    protected string name;
    public constructor(string n) { this.name = n; }
    public function getName(): string { return this.name; }
    abstract function sound(): string;
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
    public function sound(): string { return "woof"; }
    public function fetch(): string { return this.name + " fetches"; }
}

class Cat extends Animal {
    public constructor(string n) : super(n) {}
    public function sound(): string { return "meow"; }
    public function purr(): string { return this.name + " purrs"; }
}

class Cow extends Animal {
    public constructor(string n) : super(n) {}
    public function sound(): string { return "moo"; }
}

function main(): void {
    print("=== Combo 29: Array Casting + Generics ===");

    ArrayList<Animal> zoo = new ArrayList<Animal>();
    zoo.add(new Dog("Rex"));
    zoo.add(new Cat("Whiskers"));
    zoo.add(new Dog("Buddy"));
    zoo.add(new Cow("Bessie"));

    print("--- Successful casts ---");
    for (int i = 0; i < zoo.size(); i++) {
        Animal a = (Animal)zoo.get(i);
        if (a isClassOf Dog) {
            Dog d = (Dog)a;
            print(d.fetch() + " says " + d.sound());
        } else if (a isClassOf Cat) {
            Cat c = (Cat)a;
            print(c.purr() + " says " + c.sound());
        } else {
            print(a.getName() + " says " + a.sound());
        }
    }

    print("--- Bad cast caught ---");
    Animal cow = (Animal)zoo.get(3);
    try {
        Dog notADog = (Dog)cow;
        print("Unexpected success: " + notADog.fetch());
    } catch (Exception e) {
        print("Caught bad Dog cast");
    }

    Animal cat = (Animal)zoo.get(1);
    try {
        Cow notACow = (Cow)cat;
        print("Unexpected success: " + notACow.getName());
    } catch (Exception e) {
        print("Caught bad Cow cast");
    }

    print("=== Combo 29 Complete ===");
}

main();
