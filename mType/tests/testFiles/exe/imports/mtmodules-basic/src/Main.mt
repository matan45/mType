// Test: @alias resolved via mt_modules/ (no <Alias> in .mtproj)
import { Greet } from "@greetlib/Greet.mt";

@EntryPoint
class App {
    public static function main(string[] args): void {
        print(Greet::hello("MYT-304"));
    }
}
