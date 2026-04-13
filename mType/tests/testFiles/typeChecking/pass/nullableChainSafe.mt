// Test safe nullable chain operations

class Address {
    string city;

    constructor(string c) {
        city = c;
    }

    public function getCity(): string {
        return city;
    }
}

class Person {
    string name;
    Address? address;

    constructor(string n) {
        name = n;
        address = null;
    }

    public function setAddress(Address addr): void {
        address = addr;
    }

    public function getAddress(): Address? {
        return address;
    }

    public function hasAddress(): bool {
        return address != null;
    }
}

function main(): void {
    print("Testing nullable chain operations");

    Person person1 = new Person("Alice");
    Person person2 = new Person("Bob");

    // person1 has no address
    Address? p1Addr = person1.getAddress();
    if (p1Addr != null) {
        print("Person1 city: " + p1Addr.getCity());
    } else {
        print("Person1 has no address");
    }

    // person2 has address
    person2.setAddress(new Address("New York"));
    Address? p2Addr = person2.getAddress();
    if (p2Addr != null) {
        print("Person2 city: " + p2Addr.getCity());
    } else {
        print("Person2 has no address");
    }

    // Safe null check before chaining
    Person? person3 = null;
    if (person3 != null) {
        Address? p3Addr = person3.getAddress();
        if (p3Addr != null) {
            print("Person3 city: " + p3Addr.getCity());
        }
    } else {
        print("Person3 is null");
    }

    print("Nullable chain safe test completed");
}

main();
