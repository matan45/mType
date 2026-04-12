// Test: @EntryPoint that prints its arguments
// Expected: Pass - main receives and prints args

@EntryPoint
class App {
    public static function main(string[] args): void {
        int count = args.length;
        print("Arg count: " + count);
        for (int i = 0; i < count; i = i + 1) {
            print("arg[" + i + "] = " + args[i]);
        }
    }
}

string[] testArgs = new string[3];
testArgs[0] = "hello";
testArgs[1] = "world";
testArgs[2] = "42";
App::main(testArgs);
