// Test: Multiple generic parameters each with their own constraint
// Expected: Should compile and run successfully
import * from "../../lib/collections/ArrayList.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Serializable {
    function serialize(): string;
}

interface Cloneable<T> {
    function clone(): T;
}

class Data implements Comparable<Data>, Serializable {
    private int id;
    private string value;

    constructor(int i, string v) {
        this.id = i;
        this.value = v;
    }

    public function compareTo(Data other): int {
        if (this.id < other.id) {
            return -1;
        } else if (this.id > other.id) {
            return 1;
        }
        return 0;
    }

    public function serialize(): string {
        return "Data{id=" + this.id + ", value=" + this.value + "}";
    }

    public function getId(): int {
        return this.id;
    }
}

class Record implements Serializable, Cloneable<Record> {
    private string name;

    constructor(string n) {
        this.name = n;
    }

    public function serialize(): string {
        return "Record{name=" + this.name + "}";
    }

    public function clone(): Record {
        return new Record(this.name);
    }

    public function getName(): string {
        return this.name;
    }
}

// Class with multiple constrained generic parameters
class Repository<K extends Comparable<K>, V extends Serializable> {
    private ArrayList<K> keys;
    private ArrayList<V> values;

    constructor() {
        this.keys = new ArrayList<K>();
        this.values = new ArrayList<V>();
    }

    public function put(K key, V value): void {
        this.keys.add(key);
        this.values.add(value);
    }

    public function get(int index): V {
        return this.values.get(index);
    }

    public function size(): int {
        return this.values.size();
    }

    public function serializeAll(): void {
        for (int i = 0; i < this.values.size(); i = i + 1) {
            V value = this.values.get(i);
            print(value.serialize());
        }
    }
}

// Test multiple constrained parameters
Repository<Data, Record> repo = new Repository<Data, Record>();
repo.put(new Data(1, "first"), new Record("Alpha"));
repo.put(new Data(2, "second"), new Record("Beta"));
repo.put(new Data(3, "third"), new Record("Gamma"));

print("Repository contents:");
repo.serializeAll();

print("Multiple constrained parameters test passed!");
