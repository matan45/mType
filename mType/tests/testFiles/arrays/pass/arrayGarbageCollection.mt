// Test array garbage collection behavior
print("Testing array garbage collection");

class Data {
    int value;

    Data(int v) {
        value = v;
    }

    int getValue() {
        return value;
    }
}

// Create and discard arrays
for (int round = 0; round < 3; round++) {
    Data[] temp = new Data[5];
    for (int i = 0; i < temp.length; i++) {
        temp[i] = new Data(round * 10 + i);
    }
    print("Round " + round + " created array with first value: " + temp[0].getValue());
    // temp goes out of scope here
}

// Create array that persists
Data[] persistent = new Data[3];
persistent[0] = new Data(100);
persistent[1] = new Data(200);
persistent[2] = new Data(300);

print("Persistent array:");
for (int i = 0; i < persistent.length; i++) {
    print("  " + persistent[i].getValue());
}

print("Garbage collection test completed");
