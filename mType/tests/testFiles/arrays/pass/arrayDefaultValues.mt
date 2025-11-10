// Test default vs explicit values in arrays
print("Testing default vs explicit values");

// Test default values for primitives
int[] defaultInts = new int[3];
print("Default int values:");
for (int i = 0; i < defaultInts.length; i++) {
    print("  defaultInts[" + i + "] = " + defaultInts[i]);
}

float[] defaultFloats = new float[3];
print("Default float values:");
for (int i = 0; i < defaultFloats.length; i++) {
    print("  defaultFloats[" + i + "] = " + defaultFloats[i]);
}

bool[] defaultBools = new bool[3];
print("Default bool values:");
for (int i = 0; i < defaultBools.length; i++) {
    print("  defaultBools[" + i + "] = " + defaultBools[i]);
}

string[] defaultStrings = new string[3];
print("Default string values:");
for (int i = 0; i < defaultStrings.length; i++) {
    if (defaultStrings[i] == null) {
        print("  defaultStrings[" + i + "] = null");
    } else {
        print("  defaultStrings[" + i + "] = " + defaultStrings[i]);
    }
}

// Test explicit zero values
int[] explicitZeros = new int[3];
explicitZeros[0] = 0;
explicitZeros[1] = 0;
explicitZeros[2] = 0;
print("Explicit zero values:");
for (int i = 0; i < explicitZeros.length; i++) {
    print("  explicitZeros[" + i + "] = " + explicitZeros[i]);
}

// Test mixed default and explicit
int[] mixedValues = new int[4];
mixedValues[1] = 10;
mixedValues[3] = 30;
print("Mixed default and explicit:");
for (int i = 0; i < mixedValues.length; i++) {
    print("  mixedValues[" + i + "] = " + mixedValues[i]);
}

print("Default vs explicit values test completed");
