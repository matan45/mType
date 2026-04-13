// Test: match statement with null pattern

class User {
    private string name;
    constructor(string n) { this.name = n; }
    public function getName(): string { return this.name; }
}

function greet(User? user): void {
    match (user) {
        case null -> print("No user provided");
        case User u -> print("Hello, " + u.getName());
    }
}

function main(): void {
    User alice = new User("Alice");
    greet(alice);
    greet(null);
}
main();
