// Test: Null to interface cast
interface I {}

I? obj = null;
I casted = (I)obj;
print(casted == null);

// Expected output:
// true
