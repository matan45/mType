// Test: obj?.field on a non-null receiver returns the field value (MYT-374).

class Address {
    string city;
    constructor(string c) { city = c; }
    public function getCity(): string { return city; }
}

class Person {
    Address address;
    constructor(Address a) { address = a; }
}

function main(): void {
    Person? p = new Person(new Address("Paris"));
    Address? a = p?.address;
    if (a != null) {
        print("City: " + a.getCity());
    } else {
        print("null");
    }
}

main();

// Expected output:
// City: Paris
