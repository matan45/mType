class Address {
    string street;
    string city;
    
    constructor(string s, string c) {
        street = s;
        city = c;
    }
}

class Person {
    string name;
    Address address;
    
    constructor(string n, Address a) {
        name = n;
        address = a;
    }
}

Address addr = new Address("123 Main St", "Boston");
Person person = new Person("John", addr);

print(person.name); // John
print(person.address.street); // 123 Main St
print(person.address.city); // Boston

// Modify nested member
person.address.city = "Cambridge";
print(person.address.city); // Cambridge