import * from "@lib/src/Greeter.mt";

class AppMain {
    public static function run(): string {
        return Greeter::hello();
    }
}

print(AppMain::run());
