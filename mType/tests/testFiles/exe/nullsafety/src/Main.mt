// Test: Null Safety in standalone exe

class Container {
    private string value;

    public constructor(string v) {
        this.value = v;
    }

    public function getValue(): string {
        return this.value;
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Nullable type with null
        Container? nullable1 = null;
        print("nullable1 is null: " + (nullable1 == null));

        // Nullable type with value
        Container? nullable2 = new Container("hello");
        print("nullable2 is null: " + (nullable2 == null));

        // Null check before access
        if (nullable2 != null) {
            print("nullable2 value: " + nullable2.getValue());
        }

        // Reassign nullable to null
        nullable2 = null;
        print("nullable2 after null: " + (nullable2 == null));

        // Reassign back to value
        nullable2 = new Container("world");
        if (nullable2 != null) {
            print("nullable2 restored: " + nullable2.getValue());
        }

        // Non-nullable always has value
        Container nonNull = new Container("always here");
        print("nonNull: " + nonNull.getValue());

        print("Null safety test passed");
    }
}
