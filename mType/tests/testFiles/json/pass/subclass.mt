// Test JSON serialization/deserialization of subclass with inherited fields

import * from "../../lib/json/Json.mt";

class Animal {
    public string name;
    public int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }
}

class Dog extends Animal {
    public string breed;

    public constructor(string name, int age, string breed) : super(name, age) {
        this.breed = breed;
    }
}

// Serialize subclass
Dog d = new Dog("Rex", 5, "Labrador");
string json = Json::serialize(d);
print(json);

// Deserialize and verify both own and inherited fields
Dog restored = Json::deserializeAs(json, "Dog");
print(restored.name);
print(restored.age);
print(restored.breed);

print("Test passed");
