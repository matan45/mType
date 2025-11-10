// Test: Union type behavior through polymorphism
// Expected: Pass - simulating union types with base classes and interfaces

// Base type for union simulation
class Value {
    protected string typeName;

    public constructor(string type) {
        this.typeName = type;
    }

    public function getTypeName(): string {
        return this.typeName;
    }

    public function display(): void {
        print("Value of type: " + this.typeName);
    }
}

class IntValue extends Value {
    private int data;

    public constructor(int value) : super("Int") {
        this.data = value;
    }

    public function display(): void {
        print("Int: " + this.data);
    }

    public function getData(): int {
        return this.data;
    }
}

class StringValue extends Value {
    private string data;

    public constructor(string value) : super("String") {
        this.data = value;
    }

    public function display(): void {
        print("String: " + this.data);
    }

    public function getData(): string {
        return this.data;
    }
}

class BoolValue extends Value {
    private bool data;

    public constructor(bool value) : super("Bool") {
        this.data = value;
    }

    public function display(): void {
        if (this.data) {
            print("Bool: true");
        } else {
            print("Bool: false");
        }
    }

    public function getData(): bool {
        return this.data;
    }
}

// Test 1: Union-like variable (can hold different types)
print("Test 1: Union-like behavior");
Value val1 = new IntValue(42);
val1.display();

val1 = new StringValue("Hello");
val1.display();

val1 = new BoolValue(true);
val1.display();

// Test 2: Array of union types
print("\nTest 2: Array of union values");
Value[] values = new Value[5];
values[0] = new IntValue(10);
values[1] = new StringValue("Test");
values[2] = new BoolValue(false);
values[3] = new IntValue(20);
values[4] = new StringValue("World");

int i = 0;
while (i < 5) {
    print("Index " + i + " - Type: " + values[i].getTypeName());
    values[i].display();
    i = i + 1;
}

// Test 3: Function accepting union type
function processValue(Value v): void {
    print("Processing value of type: " + v.getTypeName());
    v.display();
}

print("\nTest 3: Function with union parameter");
processValue(new IntValue(99));
processValue(new StringValue("Union"));
processValue(new BoolValue(true));

// Test 4: Union type with conditional processing
class ValueProcessor {
    public function process(Value v): void {
        string type = v.getTypeName();
        print("Processing " + type + " type");
        v.display();
    }

    public function processArray(Value[] arr, int count): void {
        int idx = 0;
        while (idx < count) {
            this.process(arr[idx]);
            idx = idx + 1;
        }
    }
}

print("\nTest 4: Conditional union processing");
ValueProcessor processor = new ValueProcessor();
Value[] mixedValues = new Value[3];
mixedValues[0] = new IntValue(100);
mixedValues[1] = new StringValue("Mixed");
mixedValues[2] = new BoolValue(false);
processor.processArray(mixedValues, 3);

// Test 5: Nested union types
class Container<T> {
    private T item;

    public constructor(T value) {
        this.item = value;
    }

    public function getItem(): T {
        return this.item;
    }
}

print("\nTest 5: Containers with union values");
Container<Value> container1 = new Container<Value>(new IntValue(50));
container1.getItem().display();

Container<Value> container2 = new Container<Value>(new StringValue("Contained"));
container2.getItem().display();

// Test 6: Union type return values
class ValueFactory {
    public function createValue(int type, string data): Value {
        if (type == 0) {
            return new IntValue(123);
        }
        if (type == 1) {
            return new StringValue(data);
        }
        return new BoolValue(true);
    }
}

print("\nTest 6: Factory returning union types");
ValueFactory factory = new ValueFactory();
Value result1 = factory.createValue(0, "");
result1.display();

Value result2 = factory.createValue(1, "Factory");
result2.display();

Value result3 = factory.createValue(2, "");
result3.display();

print("\nUnion type tests completed");
