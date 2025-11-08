// Test serialization type compatibility
class Serializable {
    public string data;

    constructor(string d) {
        data = d;
    }

    public function serialize(): string {
        return "Serialized:" + data;
    }

    public static function deserialize(string serialized): Serializable {
        // Simple deserialization - extract data after colon
        return new Serializable(serialized);
    }
}

class DataContainer {
    public int id;
    public string content;

    constructor(int i, string c) {
        id = i;
        content = c;
    }

    public function toSerializable(): Serializable {
        string combined = id + ":" + content;
        return new Serializable(combined);
    }

    public static function fromSerializable(Serializable s): DataContainer {
        // Parse the data
        return new DataContainer(42, s.data);
    }
}

// Test serialization type compatibility
DataContainer container = new DataContainer(123, "test data");
Serializable serialized = container.toSerializable();
string output = serialized.serialize();

DataContainer restored = DataContainer::fromSerializable(serialized);
print("ID: " + restored.id);
print("Content: " + restored.content);
print("Serialization type compatibility passed");
