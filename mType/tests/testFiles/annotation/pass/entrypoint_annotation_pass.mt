// Test: @EntryPoint annotation on a class
// Expected: Pass - class has valid static main(string[] args): void

@EntryPoint
class App {
    public static function main(string[] args): void {
        print("Hello from EntryPoint");
    }
}

App::main(new string[0]);
