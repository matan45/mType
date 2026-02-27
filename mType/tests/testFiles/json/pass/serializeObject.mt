// Test JSON serialization of a simple object

import * from "../../lib/json/Json.mt";

class Person {
    public string name;
    public int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }
}

Person p = new Person("Alice", 30);
string json = Json::serialize(p);
print(json);

// Verify it contains expected data
Person restored = Json::deserializeAs(json, "Person");
print(restored.name);
print(restored.age);

print("Test passed");
