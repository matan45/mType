// MYT-274: zero-field user class skips synthesis (no fields to hash); the
// inherited Object native still applies and structural-equality contract
// holds (an instance equals itself, two distinct instances do not).

class Empty {
    public constructor() {
    }
}

Empty x = new Empty();
Empty y = new Empty();

print("self equals: " + x.equals(x));
print("self hash equals: " + (x.hashCode() == x.hashCode()));
print("distinct equals: " + x.equals(y));
