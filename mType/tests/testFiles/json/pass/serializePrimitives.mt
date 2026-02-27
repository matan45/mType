// Test JSON serialization of primitive values

import * from "../../lib/json/Json.mt";

class SimpleData {
    public int number;
    public float decimal;
    public bool flag;
    public string text;

    public constructor(int number, float decimal, bool flag, string text) {
        this.number = number;
        this.decimal = decimal;
        this.flag = flag;
        this.text = text;
    }
}

SimpleData data = new SimpleData(42, 3.14, true, "hello");
string json = Json::serialize(data);
print(json);

print("Test passed");
