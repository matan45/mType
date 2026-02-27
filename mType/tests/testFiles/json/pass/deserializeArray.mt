// Test JSON deserialization of arrays

import * from "../../lib/json/Json.mt";

class Container {
    public int[] numbers;

    public constructor() {
        this.numbers = new int[0];
    }
}

string json = "{\"__type\":\"Container\",\"numbers\":[10,20,30]}";
Container c = Json::deserializeAs(json, "Container");

print(c.numbers.length);
print(c.numbers[0]);
print(c.numbers[1]);
print(c.numbers[2]);

print("Test passed");
