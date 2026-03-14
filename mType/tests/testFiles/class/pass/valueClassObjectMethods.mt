// Test: Value classes using Object methods via native fallback
// Expected: Pass - Int, Bool, Float, String value classes inherit Object methods

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/String.mt";

// Test 1: Int — has custom equals(Int), toString(), hashCode()
Int a = new Int(42);
Int b = new Int(42);
Int c = new Int(99);

print("=== Int ===");
print("a equals b: " + a.equals(b));
print("a equals c: " + a.equals(c));
print("a toString: " + a.toString());
print("a hashCode nonzero: " + (a.hashCode() != 0));

// Test 2: Bool — has custom equals(Bool), toString(), hashCode()
Bool t = new Bool(true);
Bool f = new Bool(false);
Bool t2 = new Bool(true);

print("=== Bool ===");
print("t equals t2: " + t.equals(t2));
print("t equals f: " + t.equals(f));
print("t toString: " + t.toString());

// Test 3: Float — has custom equals(Float), toString(), hashCode()
Float x = new Float(3.14);
Float y = new Float(3.14);
Float z = new Float(2.71);

print("=== Float ===");
print("x equals y: " + x.equals(y));
print("x equals z: " + x.equals(z));
print("x toString: " + x.toString());

// Test 4: String — has custom equals(String), toString(), hashCode()
String s1 = new String("hello");
String s2 = new String("hello");
String s3 = new String("world");

print("=== String ===");
print("s1 equals s2: " + s1.equals(s2));
print("s1 equals s3: " + s1.equals(s3));
print("s1 toString: " + s1.toString());
print("s1 hashCode nonzero: " + (s1.hashCode() != 0));
