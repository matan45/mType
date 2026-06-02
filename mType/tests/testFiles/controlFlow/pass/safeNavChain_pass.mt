// MYT-374: a?.b?.c short-circuits the whole chain when any link is null,
// and also exercises ?. on a non-nullable receiver (full / empty are Person).

class Country {
    string name;
    constructor(string n) { name = n; }
    public function getName(): string { return name; }
}

class Address {
    public Country country;
    constructor(Country c) { country = c; }
}

class Person {
    public Address? address;
    constructor() { address = null; }
    public function setAddress(Address a): void { address = a; }
}

function main(): void {
    Person full = new Person();
    full.setAddress(new Address(new Country("France")));
    Country? c = full?.address?.country;
    if (c != null) {
        print("Country: " + c.getName());
    } else {
        print("no country");
    }

    Person empty = new Person();
    Country? c2 = empty?.address?.country;
    if (c2 != null) {
        print("Country: " + c2.getName());
    } else {
        print("no country");
    }
}

main();
