

class Stringifier {
    public static function describe(string type): string {
        switch (type) {
            case "button":
                return "button";
            case "label":
                return "label";
            default:
                return "other";
        }
    }
}

string result = Stringifier::describe("button");
print("value: " + result);
string result2 = Stringifier::describe("casda");
print("value: " + result2);
