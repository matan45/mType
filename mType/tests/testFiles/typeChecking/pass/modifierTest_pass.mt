// Test Modifier class with new bitwise operators

import {Modifier} from "../../lib/reflect/Modifier.mt";

function main(): void {
    // Test modifier constants
    int publicMod = Modifier::PUBLIC;
    int staticMod = Modifier::STATIC;

    // Combine modifiers using bitwise OR
    int combined = Modifier::combine(publicMod, staticMod);
    print("Combined (public | static): " + combined);

    // Check modifiers
    print("isPublic: " + Modifier::isPublic(combined));
    print("isStatic: " + Modifier::isStatic(combined));
    print("isPrivate: " + Modifier::isPrivate(combined));

    // Test bitwise AND
    int andResult = Modifier::and(combined, publicMod);
    print("AND result: " + andResult);

    // Test bitwise XOR
    int xorResult = Modifier::xor(combined, publicMod);
    print("XOR result: " + xorResult);

    // Test bitwise NOT
    int notResult = Modifier::not(0);
    print("NOT result: " + notResult);

    // Test toString
    string modStr = Modifier::toString(combined);
    print("Modifier string: " + modStr);
}

main();
