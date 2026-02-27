// Test JSON serialization of nested objects

import * from "../../lib/json/Json.mt";

class Address {
    public string street;
    public string city;

    public constructor(string street, string city) {
        this.street = street;
        this.city = city;
    }
}

class Employee {
    public string name;
    public int age;
    public Address address;

    public constructor(string name, int age, Address address) {
        this.name = name;
        this.age = age;
        this.address = address;
    }
}

Address addr = new Address("123 Main St", "Springfield");
Employee emp = new Employee("Alice", 30, addr);

string json = Json::serialize(emp);
print(json);

// Deserialize and verify nested object
Employee restored = Json::deserializeAs(json, "Employee");
print(restored.name);
print(restored.address.street);
print(restored.address.city);

print("Test passed");
