// MYT-281: a 1D array cast to 2D errors. The runtime cast handler
// rebuilds the array's full type name as `<elt>[]` and string-compares
// against the target — `int[]` vs `int[][]` fails the match and throws
// ClassCastException.
print("Testing array cast to wrong dimensions");

int[] a = new int[3];
a[0] = 1;
a[1] = 2;
a[2] = 3;

Object o = a;

// At runtime: fullName == "int[]"; target == "int[][]"; mismatch.
int[][] m = (int[][])o;

print("This should not be reached");
