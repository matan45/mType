// Test break statement outside loop - should produce error
// Break statements are only valid inside loops or switch statements

int x = 5;

if (x > 0) {
    print("x is positive");
    // ERROR: break outside loop or switch
    break;
}

print("This should not execute");
