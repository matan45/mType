// Test: overloaded methods can each throw distinct exceptions; overload
// resolution picks the right method and the matching catch handles it.
import * from "../../lib/exceptions/Exception.mt";

class Parser {
    public function parse(int n): void {
        if (n < 0) {
            throw new Exception("int<0");
        }
    }
    public function parse(string s): void {
        if (s == "") {
            throw new Exception("str empty");
        }
    }
}

function main(): void {
    Parser p = new Parser();
    try { p.parse(-3); } catch (Exception e) { print(e.getMessage()); }
    try { p.parse(""); } catch (Exception e) { print(e.getMessage()); }
    try { p.parse(5); print("int ok"); } catch (Exception e) { print("nope"); }
    try { p.parse("hi"); print("str ok"); } catch (Exception e) { print("nope"); }
}
main();
