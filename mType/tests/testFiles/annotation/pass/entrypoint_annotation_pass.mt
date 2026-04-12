// Test: @EntryPoint annotation on a class
// Expected: Pass - class has valid static main(String[] args): void

@EntryPoint
class App {
    static function main(String[] args): void {
        print("Hello from EntryPoint");
    }
}
