// Combo 29: Array casting via a generic helper + try/catch on bad casts
// Tests: array of base type holds subtype instances, generic cast helper succeeds
// for matching subtypes and throws a controlled error otherwise

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

class CastHelper {
    public static function <T> castAs(Animal a, string label): T {
        if (a isClassOf T) {
            return (T)a;
        }
        throw new Exception("Cannot cast " + a.getName() + " to " + label);
    }
}

function main(): void {
    print("=== Combo 29: Array Casting + Generics ===");

    Animal[] zoo = [
        new Dog("Rex"),
        new Cat("Whiskers"),
        new Dog("Buddy"),
        new Cow("Bessie")
    ];

    print("--- Successful casts ---");
    for (int i = 0; i < zoo.length; i++) {
        Animal a = zoo[i];
        if (a isClassOf Dog) {
            Dog d = CastHelper::<Dog>castAs(a, "Dog");
            print(d.fetch() + " says " + d.sound());
        } else if (a isClassOf Cat) {
            Cat c = CastHelper::<Cat>castAs(a, "Cat");
            print(c.purr() + " says " + c.sound());
        } else {
            print(a.getName() + " says " + a.sound());
        }
    }

    print("--- Bad cast caught ---");
    Animal cow = zoo[3];
    try {
        Dog notADog = CastHelper::<Dog>castAs(cow, "Dog");
        print("Unexpected success: " + notADog.fetch());
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
    }

    Animal cat = zoo[1];
    try {
        Cow notACow = CastHelper::<Cow>castAs(cat, "Cow");
        print("Unexpected success: " + notACow.getName());
    } catch (Exception e) {
        print("Caught: " + e.getMessage());
    }

    print("=== Combo 29 Complete ===");
}

main();
