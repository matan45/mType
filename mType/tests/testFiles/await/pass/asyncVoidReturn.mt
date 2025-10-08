// Test async function with Promise<void> return type

print("=== Async Void Return Test ===");

// Async function returning Promise<void>
function async doWork(): Promise<void> {
    print("Doing async work...");
    return null;  // void functions return null
}

// Call void async function
function async main(): Promise<void> {
    await doWork();
    print("Work completed!");
    return null;
}

main();
