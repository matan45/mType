import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Variance in method parameters
interface Handler<T> {
    function handle(T item): void;
}

class Animal {
    String species;

    public constructor(String s) {
        species = s;
    }

    public function getSpecies(): String {
        return species;
    }
}

class Bird extends Animal {
    public constructor() : super(new String("Bird")) {
    }
}

class Processor<T> {
    Handler<T> handler;

    public function setHandler(Handler<T> h): void {
        handler = h;
    }

    public function process(T item): void {
        handler.handle(item);
    }
}

function main(): void {
    Processor<Animal> processor = new Processor<Animal>();
    processor.setHandler(a -> print("Processing: " + a.getSpecies()));

    // Contravariant parameter - can pass Bird to Animal processor
    processor.process(new Bird());
}

main();
