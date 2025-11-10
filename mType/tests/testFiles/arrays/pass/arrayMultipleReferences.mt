// Test multiple references to same array
print("Testing multiple references");

int[] shared = new int[3];
shared[0] = 1;
shared[1] = 2;
shared[2] = 3;

int[] ref1 = shared;
int[] ref2 = shared;
int[] ref3 = shared;

print("Initial state:");
print("  shared[0] = " + shared[0]);

// Modify through ref1
ref1[0] = 10;
print("After ref1[0] = 10:");
print("  shared[0] = " + shared[0]);
print("  ref2[0] = " + ref2[0]);
print("  ref3[0] = " + ref3[0]);

// Modify through ref2
ref2[1] = 20;
print("After ref2[1] = 20:");
print("  shared[1] = " + shared[1]);
print("  ref1[1] = " + ref1[1]);
print("  ref3[1] = " + ref3[1]);

// Modify through ref3
ref3[2] = 30;
print("After ref3[2] = 30:");
print("  shared[2] = " + shared[2]);
print("  ref1[2] = " + ref1[2]);
print("  ref2[2] = " + ref2[2]);

print("Multiple references test completed");
