// MYT-282: array `equals` uses reference equality (bridge-pointer compare)
// per the Object semantics in lib/Object.mt and the runtime intercept.
// Two arrays with identical contents but different bridges are NOT equal;
// content equality is a separate concern (Arrays.equals).
import * from "../../../lib/Object.mt";

print("Testing array Object equals reference semantics");

int[] a = new int[3];
a[0] = 1;
a[1] = 2;
a[2] = 3;

int[] b = new int[3];
b[0] = 1;
b[1] = 2;
b[2] = 3;

Object xa = a;
Object xb = b;
Object xa_alias = a;

// Reflexive: same reference → true.
print("xa.equals(xa): " + xa.equals(xa));

// Aliased: different Value but same bridge → true.
print("xa.equals(xa_alias): " + xa.equals(xa_alias));

// Same contents, different bridges → false.
print("xa.equals(xb): " + xa.equals(xb));

// Symmetric of the prior.
print("xb.equals(xa): " + xb.equals(xa));

print("Done");
