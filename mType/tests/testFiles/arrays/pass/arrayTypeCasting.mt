// Test array type casting
print("Testing array type casting");

// Numeric type arrays
int[] ints = new int[3];
ints[0] = 10;
ints[1] = 20;
ints[2] = 30;

float[] floats = new float[3];
for (int i = 0; i < ints.length; i++) {
    floats[i] = ints[i];
}

print("Int array cast to float:");
for (int i = 0; i < floats.length; i++) {
    print("  floats[" + i + "] = " + floats[i]);
}

// Float to int (truncation)
float[] floatSource = new float[3];
floatSource[0] = 10.7;
floatSource[1] = 20.3;
floatSource[2] = 30.9;

int[] intCast = new int[3];
for (int i = 0; i < floatSource.length; i++) {
    intCast[i] = floatSource[i];
}

print("Float array cast to int (truncation):");
for (int i = 0; i < intCast.length; i++) {
    print("  intCast[" + i + "] = " + intCast[i]);
}

print("Array type casting test completed");
