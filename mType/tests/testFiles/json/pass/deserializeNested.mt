// Test JSON deserialization of nested objects

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
    public Address address;

    public constructor(string name, Address address) {
        this.name = name;
        this.address = address;
    }
}

string json = "{\"__type\":\"Employee\",\"name\":\"Alice\",\"address\":{\"__type\":\"Address\",\"street\":\"456 Oak Ave\",\"city\":\"Shelbyville\"}}";
Employee emp = Json::deserializeAs(json, "Employee");

print(emp.name);
print(emp.address.street);
print(emp.address.city);

print("Test passed");
