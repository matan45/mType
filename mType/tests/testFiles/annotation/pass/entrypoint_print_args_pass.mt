// Test: @EntryPoint that prints its arguments
// Expected: Pass - main receives and prints args

@EntryPoint
class App {
    static function main(String[] args): void {
        print("Arg count: " + args.length());
        for (int i = 0; i < args.length(); i = i + 1) {
            print("arg[" + i + "] = " + args[i]);
        }
    }
}
