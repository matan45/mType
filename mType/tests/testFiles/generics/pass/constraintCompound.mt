import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Compound bounds validation
interface Serializable {
    function serialize(): String;
}

interface Cloneable {
    function clone(): Cloneable;
}

class Data extends Serializable, Cloneable {
    String value;

    public function Data(String v) {
        value = v;
    }

    public function serialize(): String {
        return new String("Serialized: " + value);
    }

    public function clone(): Cloneable {
        return new Data(value);
    }

    public function getValue(): String {
        return value;
    }
}

class Repository<T extends Serializable> {
    T[] items;
    Int count;

    public function Repository() {
        items = new T[10];
        count = new Int(0);
    }

    public function store(T item): void {
        items[count.value] = item;
        print("Stored: " + item.serialize());
        count = new Int(count.value + 1);
    }
}

function main(): void {
    Repository<Data> repo = new Repository<Data>();
    repo.store(new Data(new String("Item1")));
    repo.store(new Data(new String("Item2")));
}

main();
