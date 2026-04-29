// MYT-220: a cast cannot launder null into a non-nullable target.
// Holder is non-nullable, so assigning (Holder)null is rejected.

class Holder {}

Holder h = (Holder)null;
