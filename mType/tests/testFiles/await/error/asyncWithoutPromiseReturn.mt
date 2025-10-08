// Error: Async function without Promise return type

class Message {
    string text;

    constructor(string t) {
        this.text = t;
    }
}

// ERROR: Async function must return Promise<T>, not just T
function async getMessage(): Message {
    Message msg = new Message("hello");
    return msg;
}

print("This should fail");
