// MYT-272: Float and String wrapper caches must not change observable
// semantics. Cached singletons (`new Float(0.0)`, `new String("foo")`,
// new String(""), etc.) must equal freshly-allocated wrappers with the
// same value, hashCode must agree, and out-of-cache values must continue
// to work via the fresh-allocation fallback.

import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Cached float constants per FloatCache::kCachedValues.
Float a = new Float(0.0);
Float b = new Float(0.0);
print("0.0 equals 0.0: " + a.equals(b));
print("0.0 hash matches: " + (a.hashCode() == b.hashCode()));

Float one = new Float(1.0);
Float negOne = new Float(-1.0);
print("1.0 equals -1.0: " + one.equals(negOne));

// Out-of-cache value: fresh allocation, semantics unchanged.
Float seven = new Float(7.5);
Float seven2 = new Float(7.5);
print("7.5 equals 7.5: " + seven.equals(seven2));
print("7.5 hash matches: " + (seven.hashCode() == seven2.hashCode()));

// Cached String singletons.
String empty1 = new String("");
String empty2 = new String("");
print("empty equals empty: " + empty1.equals(empty2));

String hello1 = new String("hello");
String hello2 = new String("hello");
print("hello equals hello: " + hello1.equals(hello2));
print("hello hash matches: " + (hello1.hashCode() == hello2.hashCode()));

// Out-of-cache (long) string: still works via fresh allocation.
String long1 = new String("this-string-is-longer-than-the-cache-cap-of-sixteen-bytes");
String long2 = new String("this-string-is-longer-than-the-cache-cap-of-sixteen-bytes");
print("long equals long: " + long1.equals(long2));
