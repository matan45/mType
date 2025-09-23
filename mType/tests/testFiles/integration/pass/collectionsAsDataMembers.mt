import "../../lib/collections/HashMap.mt";
import "../../lib/collections/HashSet.mt";
import "../../lib/primitives/Int.mt";
import "../../lib/primitives/String.mt";

// Test serialization with collections as data members
class DataContainer {
    int[] numbers;
    HashMap<String, Int> nameToAge;
    HashSet<String> uniqueNames;
    int numberCount;

    constructor() {
        this.numbers = new int[10]; // Simple array syntax
        this.nameToAge = new HashMap<String, Int>();
        this.uniqueNames = new HashSet<String>();
        this.numberCount = 0;
    }

    function addNumber(int num): void {
        if (this.numberCount < this.numbers.length) {
            this.numbers[this.numberCount] = num;
            this.numberCount++;
        }
    }

    function addPerson(String name, Int age): void {
        this.nameToAge.put(name, age);
        this.uniqueNames.add(name);
    }

    function getNumberCount(): int {
        return this.numberCount;
    }

    function getPersonAge(String name): Int {
        return this.nameToAge.get(name);
    }

    function hasUniqueName(String name): bool {
        return this.uniqueNames.contains(name);
    }
}

function main(): void {
    print("Testing collections as data members...");

    DataContainer container = new DataContainer();

    // Add some data
    container.addNumber(10);
    container.addNumber(20);
    container.addNumber(30);

    container.addPerson(new String("Alice"), new Int(25));
    container.addPerson(new String("Bob"), new Int(30));

    // Test the data
    print("Number count: " + container.getNumberCount());
    print("Alice age: " + container.getPersonAge(new String("Alice")).getValue());
    print("Bob age: " + container.getPersonAge(new String("Bob")).getValue());
    print("Has Alice: " + (container.hasUniqueName(new String("Alice")) ? "true" : "false"));
    print("Has Charlie: " + (container.hasUniqueName(new String("Charlie")) ? "true" : "false"));

    print("Collections as data members test completed");
}

main();