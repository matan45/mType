// Test importing arrays from modules (simulated with functions)
print("Testing array imports");

int[] getStandardValues() {
    int[] values = new int[5];
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 5;
    values[4] = 8;
    return values;
}

string[] getStandardNames() {
    string[] names = new string[3];
    names[0] = "Alpha";
    names[1] = "Beta";
    names[2] = "Gamma";
    return names;
}

// Import arrays
int[] fibonacci = getStandardValues();
string[] greekLetters = getStandardNames();

print("Imported fibonacci sequence:");
for (int i = 0; i < fibonacci.length; i++) {
    print("  " + fibonacci[i]);
}

print("Imported greek letters:");
for (int i = 0; i < greekLetters.length; i++) {
    print("  " + greekLetters[i]);
}

print("Array imports test completed");
