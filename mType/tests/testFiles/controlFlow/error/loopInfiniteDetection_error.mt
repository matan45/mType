// Test infinite loop detection - should timeout or error
// This is intentionally an infinite loop to test error handling
int counter = 0;
while (true) {
    counter = counter + 1;
    // No break condition - infinite loop
    if (counter > 10000000) {
        // This should never be reached in reasonable time
        break;
    }
}
print("Should not reach here");
