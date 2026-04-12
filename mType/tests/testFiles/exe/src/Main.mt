@EntryPoint
class App {
    public static function main(string[] args): void {
        print("Hello from standalone exe!");
        int count = args.length;
        print("Arg count: " + count);
        for (int i = 0; i < count; i = i + 1) {
            print("arg[" + i + "] = " + args[i]);
        }
    }
}
