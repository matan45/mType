import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test: Multiple bound constraints violation
// Expected: Error - type parameter violates one or more bounds

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Serializable {
    function serialize(): string;
    function deserialize(string data): void;
}

interface Cloneable {
    function clone(): Cloneable;
}

class DataRecord implements Comparable<DataRecord>, Serializable {
    int id;
    string data;

    constructor(int recordId, string recordData) {
        id = recordId;
        data = recordData;
    }

    public function compareTo(DataRecord other): int {
        if (id < other.id) {
            return -1;
        } else if (id > other.id) {
            return 1;
        }
        return 0;
    }

    public function serialize(): string {
        return id + ":" + data;
    }

    public function deserialize(string serialized): void {
        print("Deserializing: " + serialized);
    }

    public function getId(): int {
        return id;
    }
}

class SimpleRecord implements Comparable<SimpleRecord> {
    int id;

    constructor(int recordId) {
        id = recordId;
    }

    public function compareTo(SimpleRecord other): int {
        if (id < other.id) {
            return -1;
        } else if (id > other.id) {
            return 1;
        }
        return 0;
    }

    public function getId(): int {
        return id;
    }
}

class BasicData implements Serializable {
    string content;

    constructor(string c) {
        content = c;
    }

    public function serialize(): string {
        return content;
    }

    public function deserialize(string data): void {
        content = data;
    }
}

// Generic class requiring THREE interfaces
class TripleBoundedContainer<T extends Comparable<T>, T extends Serializable, T extends Cloneable> {
    T item;

    constructor(T i) {
        item = i;
    }

    public function getItem(): T {
        return item;
    }

    public function compare(T other): int {
        return item.compareTo(other);
    }

    public function save(): string {
        return item.serialize();
    }

    public function duplicate(): T {
        return item.clone();
    }
}

function main(): void {
    DataRecord record1 = new DataRecord(1, "First");
    DataRecord record2 = new DataRecord(2, "Second");

    // ERROR: DataRecord implements Comparable and Serializable but NOT Cloneable
    // This violates the third bound constraint T extends Cloneable
    TripleBoundedContainer<DataRecord> container1 = new TripleBoundedContainer<DataRecord>(record1);
    print("Container 1: " + container1.save());

    // ERROR: SimpleRecord implements Comparable but NOT Serializable or Cloneable
    SimpleRecord simple = new SimpleRecord(10);
    TripleBoundedContainer<SimpleRecord> container2 = new TripleBoundedContainer<SimpleRecord>(simple);
    print("Container 2: " + container2.getItem().getId());

    // ERROR: BasicData implements Serializable but NOT Comparable or Cloneable
    BasicData basic = new BasicData("test");
    TripleBoundedContainer<BasicData> container3 = new TripleBoundedContainer<BasicData>(basic);
    print("Container 3: " + container3.save());
}

main();
