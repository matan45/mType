import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Transitive constraints through inheritance
interface Base {
    function getId(): String;
}

interface Extended extends Base {
    function getName(): String;
}

class Implementation implements Extended {
    String id;
    String name;

    public constructor(String i, String n) {
        id = i;
        name = n;
    }

    public function getId(): String {
        return id;
    }

    public function getName(): String {
        return name;
    }
}

class Holder<T extends Extended> {
    T item;

    public function setItem(T i): void {
        item = i;
    }

    public function printInfo(): void {
        // Can use both Extended and Base methods
        print("ID: " + item.getId());
        print("Name: " + item.getName());
    }
}

function main(): void {
    Holder<Implementation> holder = new Holder<Implementation>();
    holder.setItem(new Implementation(new String("123"), new String("Test")));
    holder.printInfo();
}

main();
