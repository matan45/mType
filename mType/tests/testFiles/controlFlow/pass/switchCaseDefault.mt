// Test 1: match case
int x = 2;
switch (x) {
    case 1:
        print(100);
    case 2:
        print(200);
    case 3:
        print(300);
    default:
        print(999);
}

// Test 2: no match (default case)
int y = 5;
switch (y) {
    case 1:
        print(100);
    case 2:
        print(200);
    case 3:
        print(300);
    default:
        print(999);
}
print("Test passed"); // Test completed