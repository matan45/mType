// MYT-281: `int[]` widens directly to Object (no Box<int>[] indirection).
// The array value carries the heap reference; element types stay primitive
// at storage time and are only auto-boxed if an element is itself
// extracted into an Object slot.
print("Testing array boxing for primitive elements");

int[] ints = new int[3];
ints[0] = 11;
ints[1] = 22;
ints[2] = 33;

Object o = ints;

// Round-trip back to int[]; primitive element values must round-trip
// without boxing-induced corruption.
int[] back = (int[])o;
print("Element 0: " + back[0]);
print("Element 1: " + back[1]);
print("Element 2: " + back[2]);

// Same exercise for float[] — primitive widening preservation.
float[] floats = new float[2];
floats[0] = 1.5;
floats[1] = 2.5;
Object o2 = floats;
float[] backF = (float[])o2;
print("Float element 0: " + backF[0]);
print("Float element 1: " + backF[1]);

print("Done");
