// MYT-281/282: an array passes through multiple `Object` parameter
// signatures and returns identity-preserved each time. Combines the
// FunctionCallHelper widening, the parameter validator widening, and
// the return-type widening in a single round-trip chain.
import * from "../../../lib/Object.mt";

print("Testing array passed through Object parameters");

function describe(Object x): string {
    return x.getClass();
}

function passThrough(Object x): Object {
    return x;
}

int[] a = new int[2];
a[0] = 100;
a[1] = 200;

print("describe(int[]): " + describe(a));

string[] s = new string[1];
s[0] = "x";
print("describe(string[]): " + describe(s));

// Round-trip through passThrough preserves identity (same bridge).
Object roundTrip = passThrough(a);
int[] back = (int[])roundTrip;
print("After roundtrip back[0]: " + back[0]);
print("Length preserved: " + back.length);

print("Done");
