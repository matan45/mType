// CANARY (MYT-388): a JSON string value deserialized into an int-typed field
// must raise a type error. Currently JsonDeserializer sets the field directly
// without checking the declared type, so d.x silently becomes the string
// "not-a-number" and no error fires. Stays failing until MYT-388 lands.
import * from "../../lib/json/Json.mt";

class Dummy {
    public int x;

    public constructor() {
        this.x = 0;
    }
}

string mismatched = "{\"x\": \"not-a-number\"}";
Dummy d = Json::deserializeAs(mismatched, "Dummy");
