// Test continue statement outside loop - should produce error
// Continue statements are only valid inside loops

int x = 10;

if (x > 5) {
    print("x is greater than 5");
    // ERROR: continue outside loop
    continue;
}

print("This should not execute");
