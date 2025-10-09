// Error: Using await outside async function

class Promise<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    function getValue(): T {
        return this.value;
    }
}

class Message {
    string text;

    constructor(string t) {
        this.text = t;
    }
}

function async getMessage(): Promise<Message> {
    Message msg = new Message("hello");
    return new Promise<Message>(msg);
}

// ERROR: Cannot use await in non-async function
function processMessage(): Message {
    Promise<Message> p = await getMessage();
    return p.getValue();
}

print("This should fail");
