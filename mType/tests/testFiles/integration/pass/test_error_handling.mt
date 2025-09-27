// Error Handling Test
// Tests improved domain-specific exception handling

function testTypeErrors():void {
    print("=== Testing Type Error Handling ===");

    class TestClass {
        int value;
        constructor(int v) { this.value = v; }
        function toString():string { return "TestClass(" + value + ")"; }
    }

    // Test basic type operations that should work
    TestClass obj = new TestClass(42);
    print("Valid object creation: " + obj.toString());

    // Test null handling
    TestClass nullObj = null;
    print("Null object assignment successful");

    if (nullObj == null) {
        print("Null check working properly");
    }
}

function testArrayErrors():void {
    print("=== Testing Array Error Handling ===");

    // Test valid array operations
    int[] validArray = new int[5];
    validArray[0] = 10;
    validArray[4] = 50;

    print("Valid array operations:");
    print("  array[0] = " + validArray[0]);
    print("  array[4] = " + validArray[4]);
    print("  array length = " + validArray.length);

    // Test multi-dimensional arrays
    int[][] matrix = new int[3][3];
    matrix[1][1] = 99;
    print("2D array access: matrix[1][1] = " + matrix[1][1]);
}

function testGenericErrors():void {
    print("=== Testing Generic Type Error Handling ===");

    class Container<T> {
        T data;

        constructor(T item) {
            this.data = item;
        }

        function get():T {
            return this.data;
        }

        function toString():string {
            return "Container(" + data.toString() + ")";
        }
    }

    class SimpleValue {
        int value;
        constructor(int v) { this.value = v; }
        function toString():string { return "" + value; }
    }

    class TextValue {
        string text;
        constructor(string t) { this.text = t; }
        function toString():string { return text; }
    }

    // Test valid generic operations
    Container<SimpleValue> valueContainer = new Container<SimpleValue>(new SimpleValue(123));
    Container<TextValue> textContainer = new Container<TextValue>(new TextValue("test"));

    print("Valid generic operations:");
    print("  " + valueContainer.toString());
    print("  " + textContainer.toString());

    // Test retrieval
    SimpleValue retrievedValue = valueContainer.get();
    TextValue retrievedText = textContainer.get();

    print("Retrieved values:");
    print("  value: " + retrievedValue.toString());
    print("  text: " + retrievedText.toString());
}

function testMethodErrors():void {
    print("=== Testing Method Error Handling ===");

    class Calculator {
        function add(int a, int b):int {
            return a + b;
        }

        function divide(int a, int b):int {
            if (b == 0) {
                print("Division by zero detected!");
                return 0;
            }
            return a / b;
        }

        function toString():string {
            return "Calculator";
        }
    }

    Calculator calc = new Calculator();

    // Test valid method calls
    int sum = calc.add(10, 20);
    print("Addition result: " + sum);

    int quotient = calc.divide(20, 4);
    print("Division result: " + quotient);

    // Test division by zero handling
    int zeroResult = calc.divide(10, 0);
    print("Division by zero result: " + zeroResult);
}

function testComplexErrorScenarios():void {
    print("=== Testing Complex Error Scenarios ===");

    class Person {
        string name;
        int age;

        constructor(string n, int a) {
            this.name = n;
            this.age = a;
        }

        function toString():string {
            return name + "(" + age + ")";
        }
    }

    class Group<T> {
        T[] members;
        int count;

        constructor(int size) {
            this.members = new T[size];
            this.count = 0;
        }

        function add(T member):void {
            if (this.count < this.members.length) {
                this.members[this.count] = member;
                this.count++;
            } else {
                print("Group is full, cannot add more members");
            }
        }

        function get(int index):T {
            if (index >= 0 && index < this.count) {
                return this.members[index];
            }
            print("Invalid index: " + index);
            return null;
        }

        function size():int {
            return this.count;
        }
    }

    // Test complex nested operations
    Group<Person> personGroup = new Group<Person>(3);

    personGroup.add(new Person("Alice", 25));
    personGroup.add(new Person("Bob", 30));
    personGroup.add(new Person("Charlie", 35));

    print("Person group operations:");
    print("  Group size: " + personGroup.size());

    for (int i = 0; i < personGroup.size(); i++) {
        Person p = personGroup.get(i);
        if (p != null) {
            print("  [" + i + "] = " + p.toString());
        }
    }

    // Test adding to full group
    personGroup.add(new Person("David", 40));

    // Test invalid index access
    Person invalidPerson = personGroup.get(99);
    if (invalidPerson == null) {
        print("Properly handled invalid index access");
    }
}

function main():void {
    print("Starting Error Handling Tests...");

    testTypeErrors();
    testArrayErrors();
    testGenericErrors();
    testMethodErrors();
    testComplexErrorScenarios();

    print("Error handling tests completed!");
}

main();