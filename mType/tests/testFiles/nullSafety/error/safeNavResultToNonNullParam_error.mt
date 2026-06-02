// Error: the result of a safe-navigation access is nullable, so passing it to
// a non-nullable parameter must be rejected at compile time (MYT-374).

class Address {
    string city;
    constructor(string c) { city = c; }
}

class Person {
    Address address;
    constructor(Address a) { address = a; }
}

function useAddress(Address a): void {
    print(a.city);
}

function main(): void {
    Person? p = new Person(new Address("Paris"));
    useAddress(p?.address); // p?.address is nullable -> rejected
}

main();
