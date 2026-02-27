// Test JSON serialization with static fields

import * from "../../lib/json/Json.mt";

class Config {
    public static int maxRetries = 3;
    public static string endpoint = "https://api.example.com";
    public string name;

    public constructor(string name) {
        this.name = name;
    }
}

Config cfg = new Config("production");
Config::maxRetries = 5;

// Without static fields
string json1 = Json::serialize(cfg);
print(json1);

// With static fields
string json2 = Json::serializeWithOptions(cfg, true, true);
print(json2);

print("Test passed");
