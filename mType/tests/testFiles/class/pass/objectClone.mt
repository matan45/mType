// Test: Clone/copy semantics for objects
// Expected: Pass - demonstrates object copying

class Address {
    private string street;
    private string city;

    public constructor(string street, string city) {
        this.street = street;
        this.city = city;
    }

    public function clone(): Address {
        print("Cloning address: " + this.street + ", " + this.city);
        return new Address(this.street, this.city);
    }

    public function setStreet(string street): void {
        this.street = street;
    }

    public function toString(): string {
        return this.street + ", " + this.city;
    }
}

class Person {
    private string name;
    private int age;
    private Address address;

    public constructor(string name, int age, Address address) {
        this.name = name;
        this.age = age;
        this.address = address;
    }

    // Shallow copy - shares address reference
    public function shallowCopy(): Person {
        print("Shallow copying person: " + this.name);
        return new Person(this.name, this.age, this.address);
    }

    // Deep copy - clones address
    public function deepCopy(): Person {
        print("Deep copying person: " + this.name);
        return new Person(this.name, this.age, this.address.clone());
    }

    public function setName(string name): void {
        this.name = name;
    }

    public function getAddress(): Address {
        return this.address;
    }

    public function toString(): string {
        return this.name + " (age " + this.age + ") at " + this.address.toString();
    }
}

// Test object cloning
print("Test 1: Create original");
Address addr = new Address("123 Main St", "Springfield");
Person original = new Person("John", 30, addr);
print("Original: " + original.toString());

print("\nTest 2: Shallow copy");
Person shallow = original.shallowCopy();
print("Shallow: " + shallow.toString());
print("Same address reference: " + (original.getAddress() == shallow.getAddress()));

print("\nTest 3: Modify shallow copy's address");
shallow.getAddress().setStreet("456 Oak Ave");
print("Original: " + original.toString());
print("Shallow: " + shallow.toString());

print("\nTest 4: Deep copy");
Person deep = original.deepCopy();
print("Deep: " + deep.toString());
print("Different address reference: " + (original.getAddress() != deep.getAddress()));

print("\nTest 5: Modify deep copy's address");
deep.getAddress().setStreet("789 Pine Rd");
print("Original: " + original.toString());
print("Deep: " + deep.toString());
