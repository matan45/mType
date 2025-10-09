// Error: Async function with void return type (should be Promise<void>)

// ERROR: Async function must return Promise<T>, not void
function async doSomething(): void {
    int x = 5;
    print("Doing something");
}

print("This should fail");
