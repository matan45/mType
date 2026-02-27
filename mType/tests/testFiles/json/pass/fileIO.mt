// Test Json::writeToFile and Json::readFromFile

import * from "../../lib/json/Json.mt";

class Config {
    public string host;
    public int port;

    public constructor(string host, int port) {
        this.host = host;
        this.port = port;
    }
}

// Write to file
Config c = new Config("localhost", 8080);
Json::writeToFile("test_output_config.json", c);

// Read back from file
Config loaded = Json::readFromFile("test_output_config.json", "Config");
print(loaded.host);
print(loaded.port);

print("Test passed");
