// MYT-219: switch case with `break;` after `return ...;` makes the function's
// return type analyze as void at the call site.
//
// EXPECTED:
//   The function's declared return type (string) is honored. Call site sees
//   `string` and concatenation works. Output: "value: button"
//
// ACTUAL (broken):
//   Type error: Method '...' parameter 1 expects string but got void
//   The unreachable `break;` after `return` causes the type-flow analyzer to
//   infer a void path through the function.

class Stringifier {
    public static function describe(string type): string {
        switch (type) {
            case "button":
                return "button";
                break;
            case "label":
                return "label";
                break;
            default:
                return "other";
                break;
        }
    }
}

string result = Stringifier::describe("button");
print("value: " + result);
