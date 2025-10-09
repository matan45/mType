// Error: Async function returning primitive type

class Promise<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }
}

// ERROR: Promise<string> uses primitive type 'string' as generic argument
function async getMessage(): Promise<string> {
    return new Promise<string>("hello");
}

print("This should fail");
