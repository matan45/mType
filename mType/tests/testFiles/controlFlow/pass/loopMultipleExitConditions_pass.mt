// Test loops with multiple exit conditions

// Multiple break conditions in while loop
int count = 0;
int sum = 0;
while (true) {
    count = count + 1;
    sum = sum + count;

    // Exit condition 1: count reaches limit
    if (count >= 10) {
        print("Exit by count");
        break;
    }

    // Exit condition 2: sum exceeds threshold
    if (sum > 30) {
        print("Exit by sum");
        break;
    }
}
print(count);
print(sum);

// Multiple exit conditions with different paths
int x = 0;
int y = 0;
bool exitByX = false;
bool exitByY = false;

while (x < 20 && y < 20) {
    x = x + 2;
    y = y + 3;

    if (x > 12) {
        exitByX = true;
        break;
    }

    if (y > 15) {
        exitByY = true;
        break;
    }
}

if (exitByX) {
    print("Exited by X");
}
if (exitByY) {
    print("Exited by Y");
}

// Complex condition loop
int i = 0;
while (i < 100) {
    i = i + 1;

    if (i % 7 == 0 && i > 20) {
        print(i);
        break;
    }
}

print("Test passed");
