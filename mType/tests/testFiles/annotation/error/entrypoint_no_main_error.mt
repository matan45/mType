// Test: @EntryPoint class without main method should fail
// Expected: Error - @EntryPoint requires static main(String[] args): void

@EntryPoint
class App {
    static function run(): void {
        print("No main method");
    }
}
