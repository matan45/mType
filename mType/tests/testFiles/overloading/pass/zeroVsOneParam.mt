// Test: Zero-parameter vs one-parameter overload boundary
class Logger {
    constructor() {}

    public function log(): string {
        return "no args";
    }

    public function log(string msg): string {
        return "string: " + msg;
    }

    public function log(int level): string {
        return "level: " + level;
    }
}

function main(): void {
    print("=== Zero vs One Param ===");

    Logger l = new Logger();
    print(l.log());
    print(l.log("error"));
    print(l.log(3));
}
main();
