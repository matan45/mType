// Test async function with Promise<void> return type

print("=== Async Void Return Test ===");

// Async function returning Promise<void> with return null;
function async doWork1(): Promise<void> {
    print("Doing async work 1...");
    return null;  // explicit return null
}

// Async function returning Promise<void> with return;
function async doWork2(): Promise<void> {
    print("Doing async work 2...");
    return;  // just return; (like regular void functions)
}

// Async function with implicit return (no return statement)
function async doWork3(): Promise<void> {
    print("Doing async work 3...");
    // implicit return at end of function
}

// Call void async functions
function async main(): Promise<void> {
    await doWork1();
    print("Work 1 completed!");

    await doWork2();
    print("Work 2 completed!");

    await doWork3();
    print("Work 3 completed!");

    return;  // Test return; in main as well
}

main();
