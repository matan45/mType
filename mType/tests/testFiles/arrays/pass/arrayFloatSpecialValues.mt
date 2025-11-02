// Test NaN, Infinity, -Infinity in float arrays
print("Testing float special values in arrays");

float[] specialValues = new float[6];
print("Created float array with length: " + specialValues.length);

// Assign special float values
specialValues[0] = 0.0 / 0.0;  // NaN
specialValues[1] = 1.0 / 0.0;  // Infinity
specialValues[2] = -1.0 / 0.0; // -Infinity
specialValues[3] = 3.14159;
specialValues[4] = -2.71828;
specialValues[5] = 0.0;

// Display values
for (int i = 0; i < specialValues.length; i++) {
    print("specialValues[" + i + "] = " + specialValues[i]);
}

// Test comparisons with special values
print("Testing comparisons:");
print("Is specialValues[0] NaN? " + (specialValues[0] != specialValues[0]));
print("Is specialValues[1] > 1000? " + (specialValues[1] > 1000.0));
print("Is specialValues[2] < -1000? " + (specialValues[2] < -1000.0));

// Test operations with special values
float[] results = new float[3];
results[0] = specialValues[1] + specialValues[2]; // Infinity + (-Infinity) = NaN
results[1] = specialValues[1] * 2.0;              // Infinity * 2 = Infinity
results[2] = specialValues[2] / 2.0;              // -Infinity / 2 = -Infinity

print("Operations with special values:");
for (int i = 0; i < results.length; i++) {
    print("results[" + i + "] = " + results[i]);
}

print("Float special values test completed");
