// MYT-282: `hashCode` is identity-based for arrays — repeated calls on
// the same Value (or the same array via different aliases) return the
// same integer. Different arrays generally hash to different values
// (not guaranteed by the spec, but typically the case for distinct
// bridge pointers).
import * from "../../../lib/Object.mt";

print("Testing array Object hashCode stability");

int[] a = new int[3];
a[0] = 1;
a[1] = 2;
a[2] = 3;

Object x = a;

int h1 = x.hashCode();
int h2 = x.hashCode();
print("Stable across calls: " + (h1 == h2));

// Same bridge accessed through a different Object alias.
Object y = a;
int h3 = y.hashCode();
print("Stable across aliases: " + (h1 == h3));

print("Done");
