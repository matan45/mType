import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Declaration-site variance simulation
class ReadOnlyBox<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    // Only provides reading capability
    public function getValue(): T {
        return value;
    }
}

class Animal {
    String type;

    public constructor(String t) {
        type = t;
    }

    public function getType(): String {
        return type;
    }
}

class Cat extends Animal {
    public constructor() : super(new String("Cat")) {
    }
}

function main(): void {
    ReadOnlyBox<Cat> catBox = new ReadOnlyBox<Cat>();
    catBox.setValue(new Cat());

    // Declaration-site variance allows covariant reading
    ReadOnlyBox<Animal> animalBox = catBox;
    Animal a = animalBox.getValue();
    print("Type: " + a.getType());
}

main();
