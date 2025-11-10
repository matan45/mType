// Test loop with counter approaching large values
// Testing behavior near integer boundaries

// Start with a large number and iterate a few times
int largeNum = 2147483640; // Close to max int32
int iterations = 0;

while (iterations < 5) {
    largeNum = largeNum + 1;
    iterations = iterations + 1;
    print(largeNum);
}

// Test countdown from high value
int highValue = 2147483645;
int countdown = 0;
while (countdown < 3) {
    print(highValue);
    highValue = highValue - 1;
    countdown = countdown + 1;
}

print("Test passed");
