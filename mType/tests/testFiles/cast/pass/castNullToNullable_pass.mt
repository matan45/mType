// MYT-220: casting null and assigning to a nullable variable is allowed.

class Holder {}

Holder? h = (Holder)null;
if (h == null) {
    print("ok");
}

// Expected output:
// ok
