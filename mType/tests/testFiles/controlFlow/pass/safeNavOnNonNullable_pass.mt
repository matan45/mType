// MYT-374: ?. is permitted on a non-nullable receiver too (always safe).

class Address {
    string city;
    constructor(string c) { city = c; }
    public function getCity(): string { return city; }
}

class Person {
    Address address;
    constructor(Address a) { address = a; }
    public function getAddress(): Address { return address; }
}

function main(): void {
    Person p = new Person(new Address("Berlin"));

    Address? a = p?.address;
    if (a != null) {
        print("Field: " + a.getCity());
    }

    Address? b = p?.getAddress();
    if (b != null) {
        print("Method: " + b.getCity());
    }

    print("done");
}

main();
