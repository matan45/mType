// Test that invalid JSON throws an error

import * from "../../lib/json/Json.mt";

class Dummy {
    public int x;

    public constructor() {
        this.x = 0;
    }
}

string badJson = "{invalid json}";
Dummy d = Json::deserializeAs(badJson, "Dummy");
