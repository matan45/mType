// Test: strings containing unicode and JSON-escaped characters (quote,
// backslash, newline, tab) survive a serialize/deserialize round trip.
import * from "../../lib/json/Json.mt";

class Doc {
    public string unicodeText;
    public string escapes;

    public constructor() {
        this.unicodeText = "";
        this.escapes = "";
    }
}

function main(): void {
    Doc d = new Doc();
    d.unicodeText = "héllo wörld — 日本語 ✓";
    d.escapes = "quote:\" backslash:\\ newline:\n tab:\t";

    string json = Json::serialize(d);
    Doc restored = Json::deserializeAs(json, "Doc");

    print(restored.unicodeText == d.unicodeText);
    print(restored.escapes == d.escapes);
    print(restored.unicodeText);
    print("Unicode round trip test passed!");
}
main();
