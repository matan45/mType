// Basic async function test
// In bytecode mode, async functions automatically wrap/unwrap Promises

class Message {
    string text;

    public constructor(string t) {
        this.text = t;
    }

    public function getText(): string {
        return this.text;
    }
}

print("=== Basic Async Function Test ===");

// Simple async function - returns Promise<Message>
// But implementation returns raw Message, bytecode wraps it
function async getMessage(): Promise<Message> {
    Message msg = new Message("Hello from async");
    return msg;
}

// Call and await
function async testBasic(): Promise<Message> {
    // await unwraps Promise<Message> to Message
    Message result = await getMessage();
    return result;  // Bytecode wraps in Promise
}

// Main function to run test
function async main(): Promise<Message> {
    Message msg = await testBasic();
    print("Result: " + msg.getText());
    return msg;
}

main();
