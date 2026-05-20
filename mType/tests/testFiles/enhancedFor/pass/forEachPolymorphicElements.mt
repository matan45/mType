// Edge: for-each over ArrayList<Animal> where each element is a different
// concrete subclass. The body invokes a virtual method through the base type
// and must dispatch to the subclass implementation per element.
import * from "../../lib/collections/ArrayList.mt";

class Animal {
    public function speak(): string {
        return "...";
    }
}

class Dog extends Animal {
    public function speak(): string {
        return "woof";
    }
}

class Cat extends Animal {
    public function speak(): string {
        return "meow";
    }
}

function main(): void {
    ArrayList<Animal> zoo = new ArrayList<Animal>();
    zoo.add(new Dog());
    zoo.add(new Cat());
    zoo.add(new Dog());
    zoo.add(new Animal());

    for (Animal a : zoo) {
        print(a.speak());
    }
}

main();

// Expected output:
// woof
// meow
// woof
// ...
