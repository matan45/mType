// Test basic array types (standalone)
print("Testing generic arrays");

// Test different array types with improved syntax
print("Creating simple arrays");

string[] stringItems = new string[3];
stringItems[0] = "apple";
stringItems[1] = "banana";
stringItems[2] = "cherry";

print("String array length: " + stringItems.length);
for (int i = 0; i < stringItems.length; i++) {
    print("Item " + i + ": " + stringItems[i]);
}

int[] intItems = new int[2];
intItems[0] = 42;
intItems[1] = 84;

print("Int array length: " + intItems.length);
print("Item 0: " + intItems[0]);
print("Item 1: " + intItems[1]);

// Test boolean arrays
bool[] flags = new bool[2];
flags[0] = true;
flags[1] = false;

print("Bool array length: " + flags.length);
for (int i = 0; i < flags.length; i++) {
    print("Flag " + i + ": " + flags[i]);
}

print("Generic arrays test completed");