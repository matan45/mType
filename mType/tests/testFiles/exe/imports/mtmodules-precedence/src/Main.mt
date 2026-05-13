// Test: explicit <Alias> in .mtproj overrides an mt_modules/ alias with the same name.
// If precedence is correct, this prints "LOCAL". If mt_modules wins, it prints "MTMODULES".
import { Thing } from "@foo/Thing.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        print(Thing::which());
    }
}
