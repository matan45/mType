// Test edge case float values in arrays
print("Testing float edge values in arrays");

float[] edgeValues = new float[6];
print("Created float array with length: " + edgeValues.length);

// Assign edge case float values
edgeValues[0] = 0.0;
edgeValues[1] = 3.14159265359;      // Pi
edgeValues[2] = -2.71828182846;     // -e
edgeValues[3] = 999999999999999.0;  // Very large positive
edgeValues[4] = -999999999999999.0; // Very large negative
edgeValues[5] = 0.0000000000001;    // Very small positive

// Display values
for (int i = 0; i < edgeValues.length; i++) {
    print("edgeValues[" + i + "] = " + edgeValues[i]);
}

// Test comparisons with edge values
print("Testing comparisons:");
print("Is edgeValues[0] zero? " + (edgeValues[0] == 0.0));
print("Is edgeValues[3] > 1000? " + (edgeValues[3] > 1000.0));
print("Is edgeValues[4] < -1000? " + (edgeValues[4] < -1000.0));

// Test operations with edge values
float[] results = new float[3];
results[0] = edgeValues[1] + edgeValues[2]; // Pi + (-e)
results[1] = edgeValues[1] * 2.0;           // 2 * Pi
results[2] = edgeValues[2] / 2.0;           // -e / 2

print("Operations with edge values:");
for (int i = 0; i < results.length; i++) {
    print("results[" + i + "] = " + results[i]);
}

print("Float edge values test completed");
