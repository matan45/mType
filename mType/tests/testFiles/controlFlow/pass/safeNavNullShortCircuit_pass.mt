// MYT-374: a null receiver makes obj?.field / obj?.method() evaluate to null
// instead of raising a NullPointerException.

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
    Person? p = null;

    Address? a = p?.address;
    if (a == null) {
        print("field got null");
    } else {
        print("City: " + a.getCity());
    }

    Address? b = p?.getAddress();
    if (b == null) {
        print("method got null");
    } else {
        print("City: " + b.getCity());
    }
}

main();
