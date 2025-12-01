// Test Modifier utility methods

import * from "../../lib/reflect/Modifier.mt";

// Create some modifier combinations
int publicMod = Modifier::PUBLIC;
int privateMod = Modifier::PRIVATE;
int protectedMod = Modifier::PROTECTED;
int staticMod = Modifier::STATIC;
int finalMod = Modifier::FINAL;
int abstractMod = Modifier::ABSTRACT;

// Test isPublic
print("isPublic(PUBLIC): " + Modifier::isPublic(publicMod));
print("isPublic(PRIVATE): " + Modifier::isPublic(privateMod));

// Test isPrivate
print("isPrivate(PRIVATE): " + Modifier::isPrivate(privateMod));
print("isPrivate(PUBLIC): " + Modifier::isPrivate(publicMod));

// Test isProtected
print("isProtected(PROTECTED): " + Modifier::isProtected(protectedMod));
print("isProtected(PUBLIC): " + Modifier::isProtected(publicMod));

// Test isStatic
print("isStatic(STATIC): " + Modifier::isStatic(staticMod));
print("isStatic(PUBLIC): " + Modifier::isStatic(publicMod));

// Test isFinal
print("isFinal(FINAL): " + Modifier::isFinal(finalMod));
print("isFinal(PUBLIC): " + Modifier::isFinal(publicMod));

// Test isAbstract
print("isAbstract(ABSTRACT): " + Modifier::isAbstract(abstractMod));
print("isAbstract(PUBLIC): " + Modifier::isAbstract(publicMod));

// Test combined modifiers
int publicStaticFinal = Modifier::combine(Modifier::combine(Modifier::PUBLIC, Modifier::STATIC), Modifier::FINAL);
print("Combined PUBLIC|STATIC|FINAL:");
print("  isPublic: " + Modifier::isPublic(publicStaticFinal));
print("  isStatic: " + Modifier::isStatic(publicStaticFinal));
print("  isFinal: " + Modifier::isFinal(publicStaticFinal));
print("  isPrivate: " + Modifier::isPrivate(publicStaticFinal));
print("  isAbstract: " + Modifier::isAbstract(publicStaticFinal));

print("Test passed");
