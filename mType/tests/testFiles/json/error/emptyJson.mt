// Test that empty JSON input throws an error

import * from "../../lib/json/Json.mt";

class Dummy {
    public int x;

    public constructor() {
        this.x = 0;
    }
}

string empty = "";
Dummy d = Json::deserializeAs(empty, "Dummy");
