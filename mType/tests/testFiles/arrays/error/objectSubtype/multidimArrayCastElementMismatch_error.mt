// MYT-281: an int multi-dim array cast to a float multi-dim array type
// errors at runtime even when ranks match. The element type lives in the
// FlatMultiArray's defaultValue_ tag (set by ArrayExecutor::buildMultiArray),
// so the reconstructed name `int[][]` does not match `float[][]` and the
// cast throws ClassCastException.
print("Testing multi-dim element-type mismatch cast");

int[][] m = new int[2][2];
m[0][0] = 1;

Object o = m;
float[][] f = (float[][])o;

print("This should not be reached");
