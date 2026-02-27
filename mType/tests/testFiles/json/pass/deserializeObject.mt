// Test JSON deserialization of a simple object

import * from "../../lib/json/Json.mt";

class Person {
    public string name;
    public int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }
}

string json = "{\"__type\":\"Person\",\"name\":\"Bob\",\"age\":25}";
Person p = Json::deserializeAs(json, "Person");

print(p.name);
print(p.age);

print("Test passed");
