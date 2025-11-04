import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Override with different type bounds
interface Printable {
    function display(): void;
}

class Base {
    public function <T extends Printable> process(T item): void {
        item.display();
    }
}

class Item implements Printable {
    String name;

    public constructor(String n) {
        name = n;
    }

    public function display(): void {
        print("Item: " + name.toString());
    }
}

class Derived extends Base {
    public function <T extends Printable> process(T item): void {
        print("Derived processing");
        item.display();
    }
}

function main(): void {
    Derived d = new Derived();
    d.process<Item>(new Item(new String("Test")));
}

main();
