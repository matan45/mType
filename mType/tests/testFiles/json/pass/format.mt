// Test Json::format (validates and pretty-prints a JSON string)

import * from "../../lib/json/Json.mt";

// Format a compact JSON string
string compact = "{\"name\":\"Alice\",\"age\":30}";
string pretty = Json::format(compact);
print(pretty);

// Format a JSON array
string arr = "[1,2,3]";
string prettyArr = Json::format(arr);
print(prettyArr);

print("Test passed");
