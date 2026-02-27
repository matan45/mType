// Test JSON serialization of arrays

import * from "../../lib/json/Json.mt";

class Container {
    public int[] numbers;
    public string[] names;

    public constructor() {
        this.numbers = new int[3];
        this.numbers[0] = 10;
        this.numbers[1] = 20;
        this.numbers[2] = 30;

        this.names = new string[2];
        this.names[0] = "Alice";
        this.names[1] = "Bob";
    }
}

Container c = new Container();
string json = Json::serialize(c);
print(json);

print("Test passed");
