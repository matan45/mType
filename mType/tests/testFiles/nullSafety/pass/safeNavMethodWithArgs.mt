// Test: obj?.method(arg) passes arguments correctly through the safe-nav
// short-circuit (args are compiled between the DUP/JUMP_IF_NULL and the call).

class Message {
    string text;
    constructor(string t) { text = t; }
    public function getText(): string { return text; }
}

class Greeter {
    string prefix;
    constructor(string p) { prefix = p; }
    public function greet(string name): Message { return new Message(prefix + name); }
}

function main(): void {
    Greeter? g = new Greeter("Hello, ");
    Message? m = g?.greet("World");
    if (m != null) {
        print(m.getText());
    } else {
        print("null");
    }

    Greeter? none = null;
    Message? m2 = none?.greet("Nobody");
    if (m2 == null) {
        print("short-circuited");
    }
}

main();

// Expected output:
// Hello, World
// short-circuited
