// Test that deserializing to an unknown class throws an error

import * from "../../lib/json/Json.mt";

string json = "{\"__type\":\"NonExistentClass\",\"x\":42}";
Json::deserialize(json);
