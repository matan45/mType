import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Variance in method parameters
class Animal {
    String species;

    public function Animal(String s) {
        species = s;
    }

    public function getSpecies(): String {
        return species;
    }
}

class Bird extends Animal {
    public function Bird() {
        super(new String("Bird"));
    }
}

class Processor<T> {
    function(T): void handler;

    public function setHandler(function(T): void h): void {
        handler = h;
    }

    public function process(T item): void {
        handler(item);
    }
}

function main(): void {
    Processor<Animal> processor = new Processor<Animal>();
    processor.setHandler(function(Animal a): void {
        print("Processing: " + a.getSpecies());
    });

    // Contravariant parameter - can pass Bird to Animal processor
    processor.process(new Bird());
}

main();
