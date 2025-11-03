// Test async main function

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";

print("=== Async Main Function Test ===");

function async fetchUserData(int userId): Promise<String> {
    print("Fetching user data for ID: " + userId);
    return new String("User-" + userId);
}

function async processUserData(string userData): Promise<Int> {
    print("Processing: " + userData);
    return new Int(1);
}

// Main function is async
function async main(): Promise<Int> {
    print("Main function started");

    string user1 = await fetchUserData(101);
    print("Got user: " + user1);

    string user2 = await fetchUserData(102);
    print("Got user: " + user2);

    Int processed = await processUserData(user1);
    print("Processed count: " + processed);

    print("Main function completed");
    return processed;
}

main();
