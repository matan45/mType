// MYT-281: widening to Object does NOT copy the array — the underlying
// bridge pointer is shared, and mutations are visible through both aliases.
print("Testing array Object identity");

int[] a = new int[3];
a[0] = 1;
a[1] = 2;
a[2] = 3;

Object o = a;

// Mutate through the original.
a[0] = 99;

// Read through the Object alias by casting back.
int[] back = (int[])o;
print("Read after mutation: " + back[0]);
print("Aliased length: " + back.length);

print("Done");
