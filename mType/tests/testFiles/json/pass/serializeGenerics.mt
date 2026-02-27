// Test JSON serialization of generic objects

import * from "../../lib/json/Json.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Box<T> {
    public T value;

    public constructor(T val) {
        this.value = val;
    }
}

Box<String> strBox = new Box<String>(new String("Hello World"));
string json = Json::serialize(strBox);
print(json);

Box<Int> intBox = new Box<Int>(new Int(42));
string json2 = Json::serialize(intBox);
print(json2);

print("Test passed");
