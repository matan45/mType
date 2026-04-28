// MYT-220: (T?) cast syntax is intentionally rejected.
// Use null directly with a T? variable instead.

class Holder {}

Holder? h = (Holder?)null;
