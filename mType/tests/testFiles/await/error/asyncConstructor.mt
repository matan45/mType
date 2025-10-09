// Error: Async constructors are not allowed
// Constructors must be synchronous to ensure object initialization is complete

class User {
    string name;
    int id;

    // ERROR: Constructors cannot be async
    public constructor async(string n, int i): Promise<void> {
        this.name = n;
        this.id = i;
    }

    public function getName(): string {
        return this.name;
    }
}

print("This should fail - async constructors not allowed");
