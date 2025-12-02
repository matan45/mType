// Test Modifier class constants

import * from "../../lib/reflect/Modifier.mt";

// Test modifier constants
print("PUBLIC = " + Modifier::PUBLIC);
print("PRIVATE = " + Modifier::PRIVATE);
print("PROTECTED = " + Modifier::PROTECTED);
print("STATIC = " + Modifier::STATIC);
print("FINAL = " + Modifier::FINAL);
print("ABSTRACT = " + Modifier::ABSTRACT);
print("ASYNC = " + Modifier::ASYNC);

// Test combining modifiers
int publicStatic = Modifier::combine(Modifier::PUBLIC, Modifier::STATIC);
print("PUBLIC | STATIC = " + publicStatic);

int privateStaticFinal = Modifier::combine(Modifier::combine(Modifier::PRIVATE, Modifier::STATIC), Modifier::FINAL);
print("PRIVATE | STATIC | FINAL = " + privateStaticFinal);

// Verify the combined values are correct
print("publicStatic equals 9: " + (publicStatic == 9));
print("privateStaticFinal equals 26: " + (privateStaticFinal == 26));

print("Test passed");
