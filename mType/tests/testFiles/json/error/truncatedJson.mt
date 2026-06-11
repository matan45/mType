// Test that truncated JSON (cut off mid-value) throws an error
import * from "../../lib/json/Json.mt";

class Dummy {
    public int x;

    public constructor() {
        this.x = 0;
    }
}

string truncated = "{\"x\": ";
Dummy d = Json::deserializeAs(truncated, "Dummy");
