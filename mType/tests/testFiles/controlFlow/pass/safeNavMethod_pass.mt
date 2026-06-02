// MYT-374: obj?.method() on a non-null receiver returns the call result.

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
    Person? p = new Person(new Address("London"));
    Address? a = p?.getAddress();
    if (a != null) {
        print("City: " + a.getCity());
    } else {
        print("null");
    }
}

main();
