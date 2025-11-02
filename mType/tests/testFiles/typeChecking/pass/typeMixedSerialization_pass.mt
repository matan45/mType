// Test serialization type compatibility
class Serializable {
    string data;

    constructor(string d) {
        data = d;
    }

    string serialize(): string {
        return "Serialized:" + data;
    }

    static Serializable deserialize(string serialized): Serializable {
        // Simple deserialization - extract data after colon
        return new Serializable(serialized);
    }
}

class DataContainer {
    int id;
    string content;

    constructor(int i, string c) {
        id = i;
        content = c;
    }

    Serializable toSerializable(): Serializable {
        string combined = toString(id) + ":" + content;
        return new Serializable(combined);
    }

    static DataContainer fromSerializable(Serializable s): DataContainer {
        // Parse the data
        return new DataContainer(42, s.data);
    }
}

// Test serialization type compatibility
DataContainer container = new DataContainer(123, "test data");
Serializable serialized = container.toSerializable();
string output = serialized.serialize();

DataContainer restored = DataContainer.fromSerializable(serialized);
print("ID: " + toString(restored.id));
print("Content: " + restored.content);
print("Serialization type compatibility passed");
