// MYT-281: multi-dimensional arrays widen to Object too. The runtime
// stores them as FlatMultiArray bridges; the Object widening is bridge-
// kind agnostic.
print("Testing multidim array as Object");

int[][] m = new int[2][3];
m[0][0] = 1;
m[0][1] = 2;
m[0][2] = 3;
m[1][0] = 4;
m[1][1] = 5;
m[1][2] = 6;

Object o = m;
int[][] back = (int[][])o;

print("back[0][0] = " + back[0][0]);
print("back[1][2] = " + back[1][2]);
print("Outer length: " + back.length);

print("Done");
