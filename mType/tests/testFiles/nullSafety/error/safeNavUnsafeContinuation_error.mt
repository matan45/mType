// Error: a safe-navigation result is nullable, so continuing the chain with a
// plain '.' (instead of '?.') on that nullable receiver must be rejected.
// The fix in real code is to write p?.address?.country (MYT-374).

class Country {
    string name;
    constructor(string n) { name = n; }
    public function getName(): string { return name; }
}

class Address {
    Country country;
    constructor(Country c) { country = c; }
}

class Person {
    Address address;
    constructor(Address a) { address = a; }
}

function main(): void {
    Person? p = new Person(new Address(new Country("France")));
    Country? c = p?.address.country; // '.country' on nullable (p?.address) -> rejected
    print(c.getName());
}

main();
