// MYT-274: zero-field user class skips synthesis (no fields to hash); the
// inherited Object native still applies. With no fields to differ on,
// structural equality is trivial: an instance equals itself, and two
// distinct instances also equal (nothing to compare).

class Empty {
    public constructor() {
    }
}

Empty x = new Empty();
Empty y = new Empty();

print("self equals: " + x.equals(x));
print("self hash equals: " + (x.hashCode() == x.hashCode()));
print("distinct equals: " + x.equals(y));
