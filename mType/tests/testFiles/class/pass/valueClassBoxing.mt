// Test: Boxing classes (Int, Float, Bool, String) as value types
// Expected: Pass - primitive wrappers work as value classes

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/String.mt";

// Int value class
Int a = new Int(42);
Int b = new Int(8);
Int sum = a.add(b);
print("Int add: " + sum.toString());
print("Int value: " + sum.getValue());

// Float value class
Float f1 = new Float(3.14);
Float f2 = new Float(2.0);
Float product = f1.multiply(f2);
print("Float multiply: " + product.toString());

// Bool value class
Bool t = new Bool(true);
Bool f = new Bool(false);
Bool result = t.and(f);
print("Bool and: " + result.toString());
Bool notResult = f.not();
print("Bool not: " + notResult.toString());

// String value class
String s1 = new String("Hello");
String s2 = new String(" World");
String concat = s1.concat(s2);
print("String concat: " + concat.toString());
print("String length: " + concat.length());

// Value class equality
Int c = new Int(42);
print("Int equality: " + a.equals(c));
