// MYT-282: toString/getClass/hashCode work for every primitive element
// type, not just int[]. Exercises the elementName lookup for string,
// bool, and float NativeArrays.
import * from "../../../lib/Object.mt";

print("Testing array Object methods across element types");

string[] s = new string[2];
s[0] = "alpha";
s[1] = "beta";
Object xs = s;
print("string toString: " + xs.toString());
print("string getClass: " + xs.getClass());

bool[] b = new bool[3];
b[0] = true;
b[1] = false;
b[2] = true;
Object xb = b;
print("bool toString: " + xb.toString());
print("bool getClass: " + xb.getClass());

float[] f = new float[1];
f[0] = 3.14;
Object xf = f;
print("float toString: " + xf.toString());
print("float getClass: " + xf.getClass());

print("Done");
