// Test JSON serialization/deserialization of fields with null values

import * from "../../lib/json/Json.mt";

class Address {
    public string street;
    public string city;

    public constructor(string street, string city) {
        this.street = street;
        this.city = city;
    }
}

class Profile {
    public string name;
    public int age;
    public Address? address;

    public constructor(string name, int age, Address? address) {
        this.name = name;
        this.age = age;
        this.address = address;
    }
}

// Serialize with null field
Profile p = new Profile("Alice", 25, null);
string json = Json::serialize(p);
print(json);

// Deserialize and verify null is preserved
Profile restored = Json::deserializeAs(json, "Profile");
print(restored.name);
print(restored.age);
print(restored.address);

// Serialize with non-null field for comparison
Address addr = new Address("123 Main St", "Springfield");
Profile p2 = new Profile("Bob", 30, addr);
string json2 = Json::serialize(p2);
print(json2);

print("Test passed");
