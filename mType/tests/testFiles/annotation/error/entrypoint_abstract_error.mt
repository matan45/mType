// Test: @EntryPoint on abstract class should fail
// Expected: Error - abstract classes cannot be @EntryPoint

@EntryPoint
abstract class App {
    public static function main(string[] args): void {
        print("Should not work");
    }
}
