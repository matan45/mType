// Test: @EntryPoint on abstract class should fail
// Expected: Error - abstract classes cannot be @EntryPoint

@EntryPoint
abstract class App {
    static function main(String[] args): void {
        print("Should not work");
    }
}
